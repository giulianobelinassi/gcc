/* IPA livepatch closure pass
   Copyright (C) 2003-2025 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "function.h"
#include "tree.h"
#include "gimple-expr.h"
#include "tree-pass.h"
#include "cgraph.h"
#include "calls.h"
#include "varasm.h"
#include "ipa-utils.h"
#include "stringpool.h"
#include "attribs.h"

#include "value-range.h"
#include "basic-block.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "gimple-pretty-print.h"
#include "gimplify.h"
#include "gimple-ssa.h"
#include "fold-const.h"
#include "tree-ssa.h"
#include "tree-ssa-operands.h"
#include "tree-into-ssa.h"
#include "tree-ssanames.h"

#include "cgraph.h"

#include "dumpfile.h"

static auto_vec<symtab_node *> to_extract;
static auto_vec<symtab_node *> to_externalize;

static bool
is_in_vector (const vec<const char *> &vec, const char *value)
{
  for (const char *x : vec) {
    if (strcmp (x, value) == 0) {
      return true;
    }
  }

  return false;
}

static auto_vec<const char *>
tokenize_string_var (const char *var, const char *needle)
{
  auto_vec<const char *> ret;

  if (var == nullptr) {
    return ret;
  }

  unsigned size = strlen (var) + 1;
  char *buf = (char *) alloca (size);
  memcpy (buf, var, size);

  const char *tok = strtok(buf, needle);
  while (tok != nullptr) {
    ret.safe_push (xstrdup (tok));
    tok = strtok (nullptr, needle);
  }

  return ret;
}

static void
debug_basic_block (basic_block bb)
{
  printf("--- begin gimple bb dump ---\n");
  gimple_stmt_iterator gsi;
  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple *stmt = gsi_stmt (gsi);
      debug_gimple_stmt (stmt);
    }
  printf("--- end   gimple bb dump ---\n");
}

class ipa_livepatch_engine
{
  public:
    ipa_livepatch_engine ()
      {
	symtab_node *node;

	auto_vec<const char *> cmdline_extract_syms = tokenize_string_var
	  (symbols_to_extract, ",");

	auto_vec<const char *> cmdline_externalize_syms = tokenize_string_var
	  (symbols_to_externalize, ",");

	FOR_EACH_SYMBOL (node)
	{
	  /* Get what to do using __attribute__((patchable_extract)) and
	     __attribute__((patchable_externalize)).  */
	  tree decl = node->decl;
	  for (tree attrs = DECL_ATTRIBUTES (decl); attrs; attrs = TREE_CHAIN (attrs))
	    {
	      if (lookup_attribute ("patchable_extract", attrs) != NULL
		  && lookup_attribute_spec (get_identifier ("patchable_extract")))
		to_extract.safe_push (node);

	      if (lookup_attribute ("patchable_externalize", attrs) != NULL
		  && lookup_attribute_spec (get_identifier ("patchable_externalize")))
		to_externalize.safe_push (node);
	    }

	  /* Look for the names passed on -fextract-symbols.  */
	  if (is_in_vector (cmdline_extract_syms, node->name ()))
	    to_extract.safe_push (node);

	  /* Look for the names passed on -fexternalize-symbols.  */
	  if (is_in_vector (cmdline_externalize_syms, node->name ()))
	    to_externalize.safe_push (node);
	}
      }

    bool must_run (void)
      {
	return (bool) to_extract.length () + to_externalize.length ();
      }

    void execute (void)
      {
	/* Closure.  */
	symtab->remove_unreachable_nodes_from (to_extract, nullptr);

	/* Externalization.  */
	externalize_variables ();

	/* Closure again.  */
	symtab->remove_unreachable_nodes_from (to_extract, nullptr);
      }

  private:

    struct new_var_content
    {
      tree pointer_var;
      tree pointer_deference;
    };

    static bool
    visit_load (gimple *stmt, tree rhs, tree arg, void *data)
    {
      struct new_var_content *new_var_info = (struct new_var_content *) data;
      tree pointer_deference = new_var_info->pointer_deference;

      /* tree object that generated the reference.  */
      tree lhs = gimple_get_lhs (stmt);

      gcc_assert (lhs && "Load of variable without left side?");

      /* Create new assing statement, hence push a new gimplifier
	 global context.  */
      push_gimplify_context ();

      /* Create a gimple_seq to hold our new stmt.  */
      gimple_seq new_seq = NULL;

      /* ... create the stmt and append it to a new gimple sequence.  */
      gimple *assign_stmt = gimplify_assign (lhs, pointer_deference, &new_seq);

      /* Replace the stmts.  */
      gimple_stmt_iterator gsi = gsi_for_stmt (stmt);
      gsi_replace_with_seq (&gsi, new_seq, false);
      /* ... and destroy the context.  */
      pop_gimplify_context (NULL);

      /* Update SSA names.  */
      update_ssa (TODO_update_ssa);

      return true;
    }

    static bool
    visit_store (gimple *stmt, tree lhs, tree arg, void *data)
    {
      struct new_var_content *new_var_info = (struct new_var_content *) data;
      tree pointer_var = new_var_info->pointer_var;

      /* Create temporary variable. */
      tree temp_var = make_ssa_name (TREE_TYPE (pointer_var));

      /* Emit a load of pointer_var to the temporary variable.  */
      gimple *load = gimple_build_assign (temp_var, pointer_var);

      /* Add the new load stmt right before the original stmt.  */
      gimple_stmt_iterator gsi = gsi_for_stmt (stmt);
      gsi_insert_before (&gsi, load, GSI_NEW_STMT);
      update_stmt (load);

      /* Replace the lhs of the original stmt with the temp variable.  */
      gimple_set_lhs (stmt, build_simple_mem_ref (temp_var));
      update_stmt (stmt);

      /* Update SSA names.  */
      update_ssa (TODO_update_ssa);

      return true;
    }

    static bool
    visit_addr (gimple *stmt, tree op, tree unk, void *data)
    {
      struct new_var_content *new_var_info = (struct new_var_content *) data;
      tree pointer_var = new_var_info->pointer_var;

      /* Set rhs to be the a simple move from the pointer rather than the address
	 take of the original variable.  */
      gimple_assign_set_rhs1 (stmt, pointer_var);

      /* Mark stmt as modified.  */
      update_stmt (stmt);

      /* Update SSA names.  */
      update_ssa (TODO_update_ssa);

      return true;
    }

    /* Externalize symbol.  On livepatch context, this means redeclaring a
       symbol `TYPE var;` as `TYPE *klpe_var;`.  For functions, this redeclares
       it as a pointer to function of same type.  Returns the created variable
       node.  */
    varpool_node *
    externalize_node (symtab_node *node)
    {
      gcc_assert (TREE_CODE (node->decl) == FUNCTION_DECL ||
		  TREE_CODE (node->decl) == VAR_DECL);

      const char *var_name = IDENTIFIER_POINTER (DECL_NAME (node->decl));

      /* Inspect it.  */
      if (dump_enabled_p ())
	dump_printf (MSG_NOTE, "About to externalize: %s\n", var_name);

      tree var_type = TREE_TYPE (node->decl);
      tree pointer_type = build_pointer_type (var_type);

      /* Craft a name to the new variable.  */
      char name[64];
      strcpy(name, "klp_");
      strcat(name, var_name);

      /* Create variable with type matching a pointer to the old variable.  */
      tree pointer_var = build_decl (DECL_SOURCE_LOCATION (node->decl), VAR_DECL,
				     get_identifier (name), pointer_type);

      /* Mark variable as having its storage space in the current compilation
	 unit.  */
      TREE_STATIC (pointer_var) = true;
      TREE_PUBLIC (pointer_var) = true;

      /* Announce the new variable to symtab.  */
      varpool_node::add (pointer_var);
      varpool_node *new_node = varpool_node::get (pointer_var);


      /* Create a deference of the new pointer variable.  */
      tree pointer_deference = build1 (INDIRECT_REF, var_type, pointer_var);

      struct new_var_content new_var_info = {
	.pointer_var = pointer_var,
	.pointer_deference = pointer_deference,
      };

      /* Rewire references to the old variable to the new one.  */
      struct ipa_ref *ref = NULL;
      for (unsigned i = 0; node->iterate_referring (i, ref); ++i)
	{
	  if (cgraph_node *cnode = dyn_cast<cgraph_node *> (ref->referring))
	    {
	      /* Push referring function to global context.  */
	      push_cfun (cnode->get_fun ());

	      /* Walk through the stmts.  */
	      debug_gimple_stmt (ref->stmt);
	      walk_stmt_load_store_addr_ops (ref->stmt, &new_var_info,
					     visit_load, visit_store, visit_addr);

	      /* Pop function out of the context.   */
	      pop_cfun ();
	    }
	}

      /* Rewrite calls to the old variable to the new one.  */
      if (cgraph_node *cnode = dyn_cast<cgraph_node *>(this))
	{
	  /* Iterate on each caller of the function to externalize.  */
	  for (cgraph_edge *edge = cnode->callers; edge; edge = edge->next_caller)
	    {
	      cgraph_node *node = edge->caller;

	      /* Push function context.  We will modify the function.  */
	      push_cfun (node->get_fun ());

	      /* Get call stmt.  */
	      gcall *call_stmt = edge->call_stmt;

	      /* Create temporary variable. */
	      tree temp_var = make_ssa_name (TREE_TYPE (pointer_var));

	      /* Emit a load of pointer_var to the temporary variable.  */
	      gimple *load = gimple_build_assign (temp_var, pointer_var);

	      /* Add the new load stmt right before the original stmt.  */
	      gimple_stmt_iterator gsi = gsi_for_stmt (call_stmt);
	      gsi_insert_before (&gsi, load, GSI_NEW_STMT);
	      update_stmt (load);

	      /* Copy function arguments.  */
	      unsigned num_args = gimple_call_num_args (call_stmt);
	      auto_vec<tree> args;
	      for (unsigned i = 0; i < num_args; i++)
		args.safe_push (gimple_call_arg (call_stmt, i));

	      /* Create new call stmt.  */
	      gcall *new_call_stmt = gimple_build_call_vec (temp_var, args);
	      gimple_set_lhs (new_call_stmt, gimple_get_lhs (call_stmt));

	      /* Replace the lhs of the original stmt with the temp variable.  */
	      gsi = gsi_for_stmt (call_stmt);
	      gsi_replace (&gsi, new_call_stmt, true);

	      update_stmt (new_call_stmt);

	      /* Fix cgraph structure.  */
	      node->create_reference (new_node, IPA_REF_LOAD, load);
	      node->create_indirect_edge (new_call_stmt, 0, profile_count::uninitialized());

	      /* Update SSA names.  */
	      update_ssa (TODO_update_ssa);

	      pop_cfun ();
	    }
	}


      /* remove node.  */
      node->remove ();
      return varpool_node::get (pointer_var);
    }


    bool externalize_variables(void)
      {
	bool ret = false;
	for (unsigned i = 0; i < to_externalize.length(); ++i)
	  {
	    to_externalize[i]->externalize ();
	    ret = true;
	  }

	return ret;
      }

    auto_vec<symtab_node *> to_extract;
    auto_vec<symtab_node *> to_externalize;
};


namespace {

const pass_data pass_data_ipa_livepatch_closure =
{
  IPA_PASS, /* type */
  "livepatch", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_CGRAPHOPT, /* tv_id */
  0, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  ( TODO_remove_functions | TODO_dump_symtab ), /* todo_flags_finish */
};

class pass_ipa_livepatch_closure : public ipa_opt_pass_d
{
public:
  pass_ipa_livepatch_closure (gcc::context *ctxt)
    : ipa_opt_pass_d (pass_data_ipa_livepatch_closure, ctxt,
		      NULL, /* generate_summary */
		      NULL, /* write_summary */
		      NULL, /* read_summary */
		      NULL, /* write_optimization_summary */
		      NULL, /* read_optimization_summary */
		      NULL, /* stmt_fixup */
		      0, /* function_transform_todo_flags_start */
		      NULL, /* function_transform */
		      NULL) /* variable_transform */
  {}

  /* opt_pass methods: */

  bool gate (function *) final override
    {
      /* Do not re-run on ltrans stage.  */
      return true; //!flag_ltrans && lp.must_run ();
    }
  unsigned int execute (function *) final override
    {
      ipa_livepatch_engine lp;
      if (!flag_ltrans && lp.must_run ())
	lp.execute();

      return 0;
    }

}; // class pass_ipa_livepatch_closure

} // anon namespace

ipa_opt_pass_d *
make_pass_ipa_livepatch_closure (gcc::context *ctxt)
{
  return new pass_ipa_livepatch_closure (ctxt);
}
