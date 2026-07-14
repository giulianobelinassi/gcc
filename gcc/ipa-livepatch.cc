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

#include <elf.h>

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
#include "dominance.h"

#include "value-range.h"
#include "basic-block.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "gimple-pretty-print.h"
#include "gimple-fold.h"
#include "gimplify.h"
#include "gimple-ssa.h"
#include "fold-const.h"
#include "tree-ssa.h"
#include "tree-ssa-operands.h"
#include "tree-into-ssa.h"
#include "tree-ssanames.h"
#include "tree-phinodes.h"

#include "ssa-iterators.h"

#include "cgraph.h"

#include "dumpfile.h"
#include "diagnostic-core.h"
#include "intl.h"
#include "hash-map.h"

#include "print-tree.h"

auto_vec<const char *> gsymbols_to_extract;
auto_vec<const char *> gsymbols_to_externalize;
auto_vec<const char *> gsymbols_to_weakly_externalize;

static bool gsymbols_to_extract_init = false;
static bool gsymbols_to_externalize_init = false;
static bool gsymbols_to_weakly_externalize_init = false;

static void
remove_node_safe (symtab_node *node)
{
  if (is_a <varpool_node *>(node))
    {
      node->remove ();
      return;
    }

  struct function *fun = DECL_STRUCT_FUNCTION (node->decl);

  /* In case we have the Control Flow Graph.  */
  if (fun && fun->cfg)
    {
      /* Check if we have computed dominator tree.  If yes, then release it.  */
      if (dom_info_available_p (fun, CDI_DOMINATORS))
	free_dominance_info (fun, CDI_DOMINATORS);

      if (dom_info_available_p (fun, CDI_POST_DOMINATORS))
	free_dominance_info (fun, CDI_POST_DOMINATORS);
    }

  node->remove ();
}

void
init_symbols_to_extract(void)
{
  if (gsymbols_to_extract_init == true)
    return;

  if (symbols_to_extract == NULL || *symbols_to_extract == '\0')
    return;

  unsigned size = strlen(symbols_to_extract) + 1;
  char buf[size];
  memcpy(buf, symbols_to_extract, size);

  const char *tok;

  tok = strtok((char*) buf, ",");
  while (tok != nullptr) {
    gsymbols_to_extract.safe_push(xstrdup(tok));
    tok = strtok(nullptr, ",");
  }

  gsymbols_to_extract_init = true;
}

void
init_symbols_to_externalize(void)
{
  if (gsymbols_to_externalize_init == true)
    return;

  if (symbols_to_externalize == NULL || *symbols_to_externalize == '\0')
    return;

  unsigned size = strlen(symbols_to_externalize) + 1;
  char buf[size];
  memcpy(buf, symbols_to_externalize, size);

  const char *tok;

  tok = strtok((char*) buf, ",");
  while (tok != nullptr) {
    gsymbols_to_externalize.safe_push(xstrdup(tok));
    tok = strtok(nullptr, ",");
  }

  gsymbols_to_externalize_init = true;
}

void
init_symbols_to_weakly_externalize(void)
{
  if (gsymbols_to_weakly_externalize_init == true)
    return;

  if (symbols_to_weakly_externalize == NULL || *symbols_to_weakly_externalize == '\0')
    return;

  unsigned size = strlen(symbols_to_weakly_externalize) + 1;
  char buf[size];
  memcpy(buf, symbols_to_weakly_externalize, size);

  const char *tok;

  tok = strtok((char*) buf, ",");
  while (tok != nullptr) {
    gsymbols_to_weakly_externalize.safe_push(xstrdup(tok));
    tok = strtok(nullptr, ",");
  }

  gsymbols_to_weakly_externalize_init = true;
}

/* Attributes of an ELF symbol parsed by readelf.  */
struct symbol_attributes
{
  /** Offset of the symbol.  */
  void *offset;

  /** Size of symbol.  */
  unsigned long size;

  /** Type of symbol.  */
  int st_type;

  /** Bind of symbol.  */
  int st_bind;

  /** Visibility of symbol.  */
  int st_vis;

  /** Version ndx.  */
  char ndx[8];

  /** Name, as provided by readelf.  */
  const char *name;

  symbol_attributes(const char *offset, const char *size, const char *st_type,
		    const char *st_bind, const char *st_vis, const char *ndx,
		    const char *name)
    {
      this->offset = (void *) strtoul (offset, NULL, 16);
      this->size = strtoul (size, NULL, 10);
      this->st_type = string_to_st_type (st_type);
      this->st_bind = string_to_st_bind (st_bind);
      this->st_vis = string_to_st_vis (st_vis);
      strncpy(this->ndx, ndx, 8);
      this->ndx[7] = '\0';
      this->name = xstrdup (name);

    }

  void print(void)
    {
      printf("offset: %lx, size: %ld, st_type: %d, st_bind: %d, st_vis: %d, ndx:"
	     "%s, name: %s\n", offset, size, st_type, st_bind, st_vis, ndx, name);
    }


  /** Methods.  */
  static int string_to_st_type (const char *str)
    {
      static const struct {
	const char *name;
	int type;
      } names_to_type_tbl[16] = {
	{ "NOTYPE", STT_NOTYPE },
	{ "OBJECT", STT_OBJECT },
	{ "FUNC",   STT_FUNC   },
	{ "SECTION", STT_SECTION },
	{ "FILE",   STT_FILE },
	{ "COMMON", STT_COMMON },
	{ "TLS", STT_TLS },
	{ "NUM", STT_NUM },
	{ "LOOS", STT_LOOS },
	{ "GNU_IFUNC", STT_GNU_IFUNC },
	{ "IFUNC", STT_GNU_IFUNC },
	{ "HIOS", STT_HIOS },
	{ "LOPROC", STT_LOPROC },
	{ "HIPROC", STT_HIPROC },
      };

      for (unsigned i = 0; i < ARRAY_SIZE(names_to_type_tbl); i++)
	{
	  if (strcmp(str, names_to_type_tbl[i].name) == 0)
	    return names_to_type_tbl[i].type;
	}

	return -1;
    }

  static int string_to_st_bind (const char *str)
    {
      static const struct {
	const char *name;
	int type;
      } names_to_type_tbl[16] = {
	{ "LOCAL", STB_LOCAL },
	{ "GLOBAL", STB_GLOBAL },
	{ "WEAK",   STB_WEAK },
	{ "NUM", STB_NUM },
	{ "LOOS",   STB_LOOS },
	{ "GNU_UNIQUE", STB_GNU_UNIQUE },
	{ "HIOS", STB_HIOS },
	{ "LOPROC", STB_LOPROC },
	{ "HIPROC", STB_HIPROC },
      };

      for (unsigned i = 0; i < ARRAY_SIZE(names_to_type_tbl); i++)
	{
	  if (strcmp(str, names_to_type_tbl[i].name) == 0)
	    return names_to_type_tbl[i].type;
	}

	return -1;
    }

  static int string_to_st_vis (const char *str)
    {
      static const struct {
	const char *name;
	int type;
      } names_to_type_tbl[16] = {
	{ "DEFAULT", STV_DEFAULT },
	{ "INTERNAL", STV_INTERNAL },
	{ "HIDDEN", STV_HIDDEN },
	{ "PROTECTED", STV_PROTECTED },
      };

      for (unsigned i = 0; i < ARRAY_SIZE(names_to_type_tbl); i++)
	{
	  if (strcmp(str, names_to_type_tbl[i].name) == 0)
	    return names_to_type_tbl[i].type;
	}

	return -1;
    }
};

/* Symbol externalization type.  There are 3 sets we must employ in order to
   call a symbol:

   NONE  : No externalization can be employed and the symbol must be copied to
           the output with its body.
   WEAK  : Externalization can be employed by removing the body of the symbol
           because it is externally visible.
   STRONG: Externalization must be employed by hacking the symbol into a
           pointer which needs to be filled by an external tool, such as
           libpulp or klp.  */
enum externalization_type
{
  EXTERNALIZATION_NONE,
  EXTERNALIZATION_STRONG,
  EXTERNALIZATION_WEAK,
};

/* Check if string a is a prefix of string b.  */
inline bool prefix(const char *a, const char *b)
{
  return !strncmp(a, b, strlen(a));
}

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

static void
promote_to_public (symtab_node *node)
{
  tree decl = node->decl;
  node->externally_visible = 1;
  node->forced_by_abi = 1;

  if (cgraph_node *cnode = dyn_cast<cgraph_node *>(node)) {
    cnode->local = 0;
  }

  /* Make sure it resolves to having it possibly used by another object file.  */
  node->resolution = LDPR_PREEMPTED_REG;

  /* Make sure the symbol is public and public visible.  */
  TREE_PUBLIC (decl) = 1;
  DECL_VISIBILITY (decl) = VISIBILITY_DEFAULT;
  DECL_VISIBILITY_SPECIFIED (decl) = 1;

}

bool
symbol_table::remove_unreachable_nodes_from(const vec<symtab_node *> &nodes, FILE *file)
{
  bool changed = false;

  /* Do nothing if no extraction was request.  */
  if (nodes.length() == 0)
    return false;

  /* Do a DFS for each node to see which nodes can we reach.  This is our
   * closure.  */
  for (unsigned i = 0; i < nodes.length(); i++)
    {
      auto_vec<symtab_node *> stack; // DFS stack.
      symtab_node *node = nodes[i];

      /* Promote symbol to public in case it is private, otherwise we may endup
         removing it because the possibility of no public symbol accessing it.  */
      promote_to_public (node);

      /* Run DFS.  */
      stack.safe_push(node);
      while (!stack.is_empty())
	{
	  node = stack.pop();
	  if (node->aux != NULL)
	    {
	      /* Already analyzed.  */
	      continue;
	    }

	  if (cgraph_node *cnode = dyn_cast<cgraph_node *>(node))
	    {
	      if (cnode->inlined_to)
		{
		  /* Seems to only be used during certain passes.  */
		  if (dump_enabled_p ())
		    dump_printf (MSG_NOTE, "node %s inlined_to %s\n", cnode->name (),
			   cnode->inlined_to->name ());
		  stack.safe_push(cnode->inlined_to);
		}

	      cgraph_edge *edge;
	      for (edge = cnode->callees; edge; edge = edge->next_callee)
		{
		  /* Forward into each edge.  */
		  cgraph_node *callee = edge->callee;
		  stack.safe_push(callee);
		}
	    }

	  struct ipa_ref *ref = NULL;
	  for (unsigned i = 0; node->iterate_reference (i, ref); ++i)
	    {
	      /* References to variables.  */
	      stack.safe_push(ref->referred);
	    }

	  node->aux = (void *) 1;
	}
    }

  /* Remove unreachable nodes.  */
  symtab_node *node;
  symtab_node *next = NULL;
  for (node = first_symbol (); node; node = next)
    {
      next = node->next;
      if (!node->aux)
	{
	  if (dump_enabled_p ())
	    dump_printf (MSG_NOTE, "removing: %s\n", node->dump_name ());
	  remove_node_safe (node);
	  changed = true;
	}
      else
	{
	  node->aux = NULL;
	}
    }

  return changed;
}


class ipa_livepatch_engine
{
  public:
    ipa_livepatch_engine ()
      : to_extract(),
	to_externalize(),
	dynsym_map(),
	symtab_map(),
	can_decide_visibility_p(false),
	analyzed_nodes()
      {}

    void
    populate_extract_and_externalize(void)
      {
	/* Clear the vectors so we can reinitialize them.  */
	to_extract.truncate(0);
	to_externalize.truncate(0);

	symtab_node *node;
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
		to_externalize.safe_push (std::make_pair(node,
							 EXTERNALIZATION_STRONG));

	      if (lookup_attribute ("patchable_weakly_externalize", attrs) != NULL
		  && lookup_attribute_spec (get_identifier ("patchable_weakly_externalize")))
		to_externalize.safe_push (std::make_pair(node,
							 EXTERNALIZATION_WEAK));
	    }

	  /* Look for the names passed on -fextract-symbols.  */
	  if (is_in_vector (gsymbols_to_extract, node->name ()))
	    to_extract.safe_push (node);

	  /* Look for the names passed on -fexternalize-symbols.  */
	  if (is_in_vector (gsymbols_to_externalize, node->name ()))
	    to_externalize.safe_push (std::make_pair(node,
						     EXTERNALIZATION_STRONG));

	  /* Look for the names passed on -fweakly-externalize-symbols.  */
	  if (is_in_vector (gsymbols_to_weakly_externalize, node->name ()))
	    to_externalize.safe_push (std::make_pair(node,
						     EXTERNALIZATION_WEAK));
	}
      }

    void
    parse_readelf_output (FILE *pipe)
      {
	hash_map<nofree_string_hash, struct symbol_attributes> *map = NULL;

	if (pipe == NULL)
	  {
	    fatal_error (input_location,
			 G_("cannot open pipe for reading"));
	  }

	size_t len = 0;
	ssize_t nread;
	char *line = NULL;

	while ((nread = getline (&line, &len, pipe)) != -1)
	  {
	    if (line[nread-1] == '\n')
	      line[nread-1] = '\0';

	    if (prefix("Symbol table", line))
	      {
		/* Check in which string table we are.  */
		char *p = line + strlen ("Symbol table ");
		char *tbl = strtok(p, " ");

		can_decide_visibility_p = true;

		if (strcmp (tbl, "'.dynsym'") == 0)
		  map = &dynsym_map;
		else if (strcmp (tbl, "'.symtab'") == 0)
		  map = &symtab_map;
	      }
	    else
	      {
		const char *num, *value, *size, *type, *bind, *vis, *ndx, *name;
		const char *p;

		num = strtok (line, " ");
		if (num && ISDIGIT(num[0]))
		  {
		    value = strtok (NULL, " ");
		    size = strtok (NULL, " ");
		    type = strtok (NULL, " ");
		    bind = strtok (NULL, " ");
		    vis = strtok (NULL, " ");
		    ndx = strtok (NULL, " ");
		    name = strtok (NULL, " ");

		    /* Check if we have a name because of lines like this:
		       0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND
		       which doesn't have a symbol name.  */
		    if (name)
		      {
			char *name_clean = xstrdup (name);
			name_clean = strtok(name_clean, "@");

			map->put (name_clean, symbol_attributes (value, size, type, bind, vis,
								 ndx, name));
		      }
		  }
	      }
	  }

	free (line);
      }

    bool load_single_livepatch_target (const char *path)
      {
	/* Setup and run readelf.  */

	const char *argv[4];
	argv[0] = "/usr/bin/readelf";
	argv[1] = "-sW";
	argv[2] = path;
	argv[3] = NULL;

	const char *envp[4];
	envp[0] = "LC_ALL=C";
	envp[1] = NULL;

	struct pex_obj *pex;
	pex = pex_init (PEX_USE_PIPES, argv[0], NULL);
	if (!pex)
	  fatal_error (input_location, "Unable to launch readelf");

	const char *errmsg;
	int err;

	errmsg = pex_run_in_environment (pex, PEX_SEARCH, argv[0],
					 (char* const*) argv, (char * const*) envp,
					 NULL, NULL, &err);

	if (errmsg)
	  {
	    errno = err;
	    fatal_error (input_location,
			 err ? G_("cannot execute %qs: %s: %m")
			 : G_("cannot execute %qs: %s"),
			 argv[0], errmsg);
	  }

	/* Parse the output, do it before waiting the process to finish,
	 * otherwise it will be interrupted due to full FIFO.  */
	parse_readelf_output (pex_read_output (pex, false));

	/* Wait for the process to finish.  */
	int status;
	int ret_code = 0;
	if (!pex_get_status (pex, 1, &status))
	  fatal_error (input_location, "failed to get exit status: %m");

	pex_free (pex);
	pex = NULL;

	return false;
      }

    /* Load the symbols in the target ELF in which contains the symbols.  This
       ensures that we know which symbols are available to be called */
    bool load_livepatch_targets (void)
      {
	auto_vec<const char *> targets = tokenize_string_var(target_binary_path,
							     ",");

	for (const char *path : targets)
	  {
	    if (dump_enabled_p ())
	      dump_printf (MSG_NOTE, "load_livepatch_targets\n");
	    load_single_livepatch_target(path);
	  }

	return false;
      }

    bool must_run (void)
      {
	return (bool) to_extract.length () + to_externalize.length ();
      }

    int execute (void)
      {
	load_livepatch_targets();

	/* Initialize the symbols we must perform analysis.  */
	populate_extract_and_externalize ();

	/* Closure.  */
	symtab->remove_unreachable_nodes_from (to_extract, nullptr);

	/* Reinitialize the extract and externalize vectors because some.
	   nodes may have been removed.  */
	populate_extract_and_externalize ();

	/* Externalization.  */
	run_externalization_process ();

	/* Update SSA names.  */
	for (struct function *fun : modified_functions)
	  {
	    /* FIXME: Properly check if the function was removed.  */
	    if (fun->decl != (void *)0xa5a5a5a5a5a5a5a5)
	      {
		push_cfun (fun);
		printf ("SSA updating %s\n", IDENTIFIER_POINTER (DECL_NAME (fun->decl)));
		update_ssa (TODO_update_ssa);
		pop_cfun ();
	      }
	  }

	/* Closure again.  */
	symtab->remove_unreachable_nodes_from (to_extract, nullptr);

	return 0;
      }

  private:

    struct new_var_content
    {
      tree pointer_var;
      tree pointer_deference;
      symtab_node *referring;
      symtab_node *reference;
    };

    static bool
    visit_load (gimple *stmt, tree rhs, tree arg, void *data)
    {
      (void) rhs;
      (void) arg;

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

#if 0
      printf ("----------\n");
      printf ("XXX\n ");
      debug_tree(rhs);
      printf ("YYY\n ");
      debug_tree(arg);
      //debug_tree (pointer_deference);
      printf ("ZZZ\n ");
      debug_gimple_stmt (stmt);
      printf ("----------\n");
#endif

      gimple *assign_stmt;

      if (TREE_CODE (arg) == COMPONENT_REF)
	{
	  /* In case our variable to externalize is doing a COMPONENT_REF (i.e.,
	     var.attr), then we have to be careful to assemble an statement that
	     also is a COMPONENT_REF.  */

	  tree field = TREE_OPERAND (arg, 1);
	  tree component = build3 (COMPONENT_REF,
				   TREE_TYPE (field),
				   pointer_deference,
				   field,
				   NULL_TREE);

	  /* ... create the stmt based on the original variable and append it
	     to a new gimple sequence.  */
	  assign_stmt = gimplify_assign (lhs, component, &new_seq);
	}
      else
	{
	  /* ... create the stmt based on the original variable and append it
	     to a new gimple sequence.  */
	  assign_stmt = gimplify_assign (lhs, pointer_deference, &new_seq);
	}

      /* Copy the VUSE from the original stmt to the new one.  */
      gimple_set_vuse (assign_stmt, gimple_vuse (stmt));

      /* Replace the stmts.  */
      gimple_stmt_iterator gsi = gsi_for_stmt (stmt);
      gsi_replace_with_seq_vops (&gsi, new_seq);

      /* Update the orginal stmt.  */
      update_stmt (stmt);

      /* Update the references in the callgraph.  */
      symtab_node *referring = new_var_info->referring;
      symtab_node *reference = new_var_info->reference;

      referring->create_reference(reference, IPA_REF_LOAD, assign_stmt);

      /* ... and destroy the context.  */
      pop_gimplify_context (NULL);

      printf("After\n");
      debug_basic_block (gimple_bb (new_seq));

      return true;
    }

    static bool
    visit_store (gimple *stmt, tree lhs, tree arg, void *data)
    {
      struct new_var_content *new_var_info = (struct new_var_content *) data;
      tree pointer_var = new_var_info->pointer_var;
      tree pointer_deference = new_var_info->pointer_deference;

      tree temp_var;
      gimple *load;

      /* In case we are handling a COMPONENT_REF we need to know in which field
         the assignment will be.  */
      if (TREE_CODE (arg) == COMPONENT_REF)
	{
	  tree field = TREE_OPERAND (arg, 1);
	  tree component = build3 (COMPONENT_REF,
				   TREE_TYPE (field),
				   //pointer_deference,
				   build_simple_mem_ref (pointer_var),
				   field,
				   NULL_TREE);

	  /* Replace left hand side with a write to the externalized
	     variable klpe_var->field.  */
	  gimple_set_lhs (stmt, component);

	  /* Update changed stmt.  */
	  update_stmt (stmt);

	  printf("After\n");
	  debug_basic_block (gimple_bb (stmt));
	  return true;
	}
      else
	{
	  /* Ordinary pointer deference.  */

	  /* Create temporary variable. */
	  temp_var = make_ssa_name (TREE_TYPE (pointer_var));

	  /* Emit a load of pointer_var to the temporary variable.  */
	  load = gimple_build_assign (temp_var, pointer_var);

	  /* Add the new load stmt right before the original stmt.  */
	  gimple_stmt_iterator gsi = gsi_for_stmt (stmt);
	  gsi_insert_before (&gsi, load, GSI_NEW_STMT);

	  /* Update the stmt.  */
	  update_stmt (load);

	  /* Replace the lhs of the original stmt with the temp variable.  */
	  gimple_set_lhs (stmt, build_simple_mem_ref (temp_var));
	  update_stmt (stmt);
	}


      /* Update the references in the callgraph.  */
      symtab_node *referring = new_var_info->referring;
      symtab_node *reference = new_var_info->reference;

      referring->create_reference(reference, IPA_REF_LOAD, load);

      /* Replace the lhs of the original stmt with the temp variable.  */
      //gimple_set_lhs (stmt, build_simple_mem_ref (temp_var));
      //update_stmt (stmt);

      printf("After\n");
      debug_basic_block (gimple_bb (stmt));

      return true;
    }

    static bool
    visit_addr (gimple *stmt, tree op, tree unk, void *data)
    {
      struct new_var_content *new_var_info = (struct new_var_content *) data;
      tree pointer_var = new_var_info->pointer_var;

      if (gcond *cond_stmt = dyn_cast <gcond *> (stmt))
	{
	  /* Create temporary variable. */
	  tree temp_var = make_ssa_name (TREE_TYPE (pointer_var));

	  /* Emit a load of pointer_var to the temporary variable.  */
	  gimple *load = gimple_build_assign (temp_var, pointer_var);

	  /* Add the new load stmt right before the original stmt.  */
	  gimple_stmt_iterator gsi = gsi_for_stmt (stmt);
	  gsi_insert_before (&gsi, load, GSI_NEW_STMT);
	  update_stmt (load);

	  /* Set rhs.  */
	  gimple_cond_set_rhs (cond_stmt, temp_var);
	}
      else if (gphi *phi_stmt = dyn_cast <gphi *> (stmt))
	{
#if 0
	  /* Create temporary variable. */
	  tree temp_var = make_ssa_name (TREE_TYPE (pointer_var));

	  /* Emit a load of pointer_var to the temporary variable.  */
	  gimple *load = gimple_build_assign (temp_var, pointer_var);

	  /* Add the new load stmt right before the original stmt.  */
	  gimple_stmt_iterator gsi = gsi_for_stmt (stmt);

	  gsi_insert_before (&gsi, load, GSI_NEW_STMT);
	  update_stmt (load);

	  /* Update PHI parameter.  */
	  for (unsigned i = 0; i < gimple_phi_num_args (phi_stmt); i++)
	    {
	      tree arg = gimple_phi_arg_def (phi_stmt, i);
	      if (TREE_CODE (arg) == ADDR_EXPR)
		{
		  tree var = TREE_OPERAND (arg, 0);
		  if (var == op)
		    {
		      /* PHI <&var, ...>, replace &var with klpe_var.  */
		      SET_PHI_ARG_DEF (phi_stmt, i, temp_var);
		    }
		}

	    }
#endif
	}
      else if (gcall *call_stmt = dyn_cast <gcall *> (stmt))
	{
	  printf ("gcall: ");
	  debug_gimple_stmt (stmt);

	  /* Create temporary variable. */
	  tree temp_var = make_ssa_name (TREE_TYPE (pointer_var));

	  /* Emit a load of pointer_var to the temporary variable.  */
	  gimple *load = gimple_build_assign (temp_var, pointer_var);

	  /* Add the new load stmt right before the original stmt.  */
	  gimple_stmt_iterator gsi = gsi_for_stmt (stmt);
	  gsi_insert_before (&gsi, load, GSI_NEW_STMT);
	  update_stmt (load);

	  //debug_tree (op);

	  unsigned nargs = gimple_call_num_args (call_stmt);
	  for (unsigned i = 0; i < nargs; i++) {
	    tree arg = gimple_call_arg (call_stmt, i);

	    //printf ("debug tree arg %d\n", i);
	    //debug_tree (arg);

	    if (arg == op) {
	      //printf ("Found: ");
	      //debug_tree (arg);
	      //exit(1);
	    }
	  }
	}
      else
	{
	  /* Set rhs to be the a simple move from the pointer rather than the address
	     take of the original variable.  */
	  gimple_assign_set_rhs1 (stmt, pointer_var);
	}

      /* Mark stmt as modified.  */
      update_stmt (stmt);

      /* Update the references in the callgraph.  */
      symtab_node *referring = new_var_info->referring;
      symtab_node *reference = new_var_info->reference;

      referring->create_reference(reference, IPA_REF_ADDR, stmt);

      printf("After\n");
      debug_basic_block (gimple_bb (stmt));

      return true;
    }

    /* Run on references of the function to externalize which are not covered
       by GIMPLE stmts, for example on C99 initializers.  */
    static tree
    tree_walk_externalizer (tree *tp, int *walk_subtrees, void *data)
    {
      struct new_var_content *new_var_info = (struct new_var_content *) data;
      /* in the following case:
	  struct AA {
	    void *fun;
	  } A = {
	    .fun = function,
	  };
	  and if we weed to externalize `function`, we need to remove the
	  reference to function.
       */
      if (TREE_CODE (*tp) == ADDR_EXPR)
	{
	  tree arg = TREE_OPERAND (*tp, 0);
	  if (VAR_OR_FUNCTION_DECL_P (arg))
	    {
	      warning_at (EXPR_LOCATION (*tp), 0,
			  "Unable to fully externalize %s: used by initializer "
			  "of %s\n",
			  IDENTIFIER_POINTER (DECL_NAME (arg)),
			  new_var_info->referring->name ());

	      /* Drop the initializer.  */
	      *tp = build_zero_cst (integer_type_node);
	      *walk_subtrees = 0;
	    }
	}
      return NULL_TREE;
    }

    void
    weakly_externalize_node (cgraph_node *node)
      {
	// drop body
	node->release_body ();
	node->reset ();
	node->body_removed = true;
	node->analyzed = 0;
	DECL_EXTERNAL (node->decl) = 1;
      }

    void
    weakly_externalize_node (varpool_node *node)
    {
      // TODO: drop the initializer and declare it as extern.
      TREE_STATIC (node->decl) = 0;
      DECL_EXTERNAL (node->decl) = 1;
    }

    void
    weakly_externalize_node (symtab_node *node)
      {
	if (cgraph_node *cnode = dyn_cast<cgraph_node *> (node))
	  {
	    weakly_externalize_node (cnode);
	    return;
	  }
	if (varpool_node *vnode = dyn_cast <varpool_node *> (node))
	  {
	    weakly_externalize_node (vnode);
	    return;
	  }

	gcc_unreachable ();
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
      strcpy(name, "klpe_");
      strcat(name, var_name);

      /* Create variable with type matching a pointer to the old variable.  */
      tree pointer_var = build_decl (DECL_SOURCE_LOCATION (node->decl), VAR_DECL,
				     get_identifier (name), pointer_type);

      /* Mark variable as having its storage space in the current compilation
	 unit.  */
      TREE_STATIC (pointer_var) = true;
      TREE_PUBLIC (pointer_var) = true;
      TREE_USED (pointer_var) = true;


      /* Announce the new variable to symtab.  */
      varpool_node::add (pointer_var);
      varpool_node *new_node = varpool_node::get (pointer_var);
      new_node->force_output = true;

      /* Copy TLS bit in case this node is a variable.  */
      if (varpool_node *vnode = dyn_cast<varpool_node *> (node))
	new_node->tls_model = vnode->tls_model;

      /* Make sure the node is marked as analyzed for
	 `remove_unreferenced_decls` not catch it as unreferenced.  */
      new_node->analyzed = true;

      /* Create a deference of the new pointer variable.  */
      tree pointer_deference = build1 (INDIRECT_REF, var_type, pointer_var);

      //debug_tree (pointer_deference);

      /* Rewire references to the old variable to the new one.  */
      struct ipa_ref *ref = NULL;
      for (unsigned i = 0; node->iterate_referring (i, ref); ++i)
	{
	  struct new_var_content new_var_info = {
	    .pointer_var = pointer_var,
	    .pointer_deference = pointer_deference,
	    .referring = ref->referring,
	    .reference = new_node,
	  };

	  if (ref->stmt)
	    {
	      printf ("Before\n");
	      basic_block bb = gimple_bb (ref->stmt);
	      debug_basic_block (bb);
	    }

	  if (cgraph_node *cnode = dyn_cast<cgraph_node *> (ref->referring))
	    {
	      /* Push referring function to global context.  */
	      push_cfun (cnode->get_fun ());
	      modified_functions.add (cnode->get_fun ());

	      /* Walk through the stmts.  */
	      //debug_gimple_stmt (ref->stmt);
	      walk_stmt_load_store_addr_ops (ref->stmt, &new_var_info,
					     visit_load, visit_store, visit_addr);

	      /* Pop function out of the context.   */
	      pop_cfun ();
	    }

	  /* The symbol could have been used as a variable initialization, or is
	   * being used as a variable reference.  */
	  if (varpool_node *vnode = dyn_cast<varpool_node *> (ref->referring))
	    {
	      tree init = DECL_INITIAL (vnode->decl);
	      walk_tree (&init, tree_walk_externalizer, (void *) &new_var_info, NULL);
	    }
	}

      /* Rewrite calls to the old variable to the new one.  */
      if (cgraph_node *cnode = dyn_cast<cgraph_node *>(node))
	{
	  /* Iterate on each caller of the function to externalize.  */
	  for (cgraph_edge *edge = cnode->callers; edge; edge = edge->next_caller)
	    {
	      cgraph_node *node = edge->caller;

	      /* Push function context.  We will modify the function.  */
	      push_cfun (node->get_fun ());
	      modified_functions.add (node->get_fun ());

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

	      pop_cfun ();
	    }
	}

      /* Mark node as externalized.  */
      externalized.add (node);

      /* remove node.  */
      remove_node_safe (node);

      return varpool_node::get (pointer_var);
    }

    void print_dynsym (void)
      {
	printf ("dynsym map:\n");
	for (auto it = dynsym_map.begin(); it != dynsym_map.end (); ++it)
	  {
	    printf("%s => ", (*it).first);
	    (*it).second.print();
	  }
      }

    void print_symtab (void)
      {
	printf ("symtab map:\n");
	for (auto it = symtab_map.begin(); it != symtab_map.end (); ++it)
	  {
	    printf("%s => ", (*it).first);
	    (*it).second.print();
	  }
      }

    /* Given a symbol with 'name', check what externalization method we must
       employ in order to call it.  */
    externalization_type get_externalization_method (const char *name)
      {
	/* If the symbol exists in the dynsym_map, then we can weakly
	 * externalize it.  */
	symbol_attributes *sym = dynsym_map.get (name);

	if (sym && sym->st_bind == STB_GLOBAL && sym->st_vis == STV_DEFAULT)
	  return EXTERNALIZATION_WEAK;

	/* If not in the dynsym, check if it is in symtab.  If yes, the symbol
	   is still there and should be callable with some additional steps.  */
	sym = symtab_map.get (name);
	if (sym && (unsigned long) sym->offset > 0)
	  return EXTERNALIZATION_STRONG;

	return EXTERNALIZATION_NONE;
      }

    externalization_type get_externalization_method (symtab_node *sym)
      {
	return get_externalization_method (sym->asm_name ());
      }

    void run_externalize_to_function (symtab_node *node)
      {
	struct ipa_ref *ref = NULL;

	if (analyzed_nodes.contains (node))
	  return;

	analyzed_nodes.add (node);

	/* Check the symbols this function references to.  A reference to a
	   symbol is something like:
	    int a = global;

	   where `global` is a global variable.  Other cases exists of
	   course.  */
	for (unsigned i = 0; node->iterate_reference (i, ref); ++i)
	  {
	    symtab_node *rnode = ref->referred;
	    /* Get externalization method to see if we can externalize this
	       symbol, or if we need to propagate further.  */
	    externalization_type e = get_externalization_method (rnode);

	    switch (e)
	      {
		case EXTERNALIZATION_STRONG:
		  externalize_node (rnode);
		  break;

		case EXTERNALIZATION_WEAK:
		  weakly_externalize_node (rnode);
		  break;

		case EXTERNALIZATION_NONE:
		  run_externalize_to_function (rnode);
	      }
	  }

	/* Rewrite calls to the old variable to the new one.  */
	if (cgraph_node *cnode = dyn_cast<cgraph_node *>(node))
	  {
	    /* Now look for function calls in this function for symbol to
	       externalize.  Iterate on each callees of the function to
	       analyze.  */
	    for (cgraph_edge *edge = cnode->callees; edge;)
	      {
		cgraph_node *node = edge->callee;

		/* Get externalization method to see if we can externalize this
		   symbol, or if we need to propagate further.  */
		externalization_type e = get_externalization_method (node);

		/* Get the next callee here as externalizing the node
		   releases the edge.  */
		edge = edge->next_callee;

		switch (e)
		  {
		  case EXTERNALIZATION_STRONG:
		    externalize_node (node);
		    break;

		  case EXTERNALIZATION_WEAK:
		    weakly_externalize_node (node);
		    break;

		  case EXTERNALIZATION_NONE:
		    run_externalize_to_function (node);
		  }
	      }
	  }
	}

    void run_externalization_process (void)
      {
	/* Look to all symbols to extract and see if we can externalize
	   symbols so our compilation unit gets smaller.  */
	for (unsigned i = 0; i < to_extract.length(); ++i)
	  {
	    run_externalize_to_function (to_extract[i]);
	  }

	/* Look for symbols that the user input as needing externalization.  */
	for (unsigned i = 0; i < to_externalize.length(); ++i)
	  {
	    symtab_node *node = to_externalize[i].first;
	    externalization_type ext = to_externalize[i].second;

	    /* If the symbol was already externalized, then skip it.  */
	    if (externalized.contains (node))
	      continue;

	    if (ext == EXTERNALIZATION_STRONG)
	      externalize_node (node);
	    else if (ext == EXTERNALIZATION_WEAK)
	      {
		/* Check if it actually makes sense to do it.  */
		if (DECL_VISIBILITY (node->decl) == VISIBILITY_DEFAULT)
		  weakly_externalize_node (node);
		else
		  error_at (DECL_SOURCE_LOCATION (node->decl), 0, "Unable to "
			    "weakly externalize %s, function is not public "
			    "visibile\n", IDENTIFIER_POINTER (DECL_NAME
							      (node->decl)));
	      }
	  }
      }

    /* Symbols to extract.  */
    auto_vec<symtab_node *> to_extract;

    /* Symbols to externalize, that means to be redeclared as a pointer to the
       original variable, or simply to have its body removed.  */
    auto_vec<std::pair<symtab_node *, externalization_type>> to_externalize;

    /* Nodes that were externalized.  */
    hash_set<symtab_node *> externalized;

    /* hash mapping the symbol name (e.g. function name) to attributes in the
       dynsym table.  This means symbols that can be called without doing
       externsive externalization hacks.  */
    hash_map<nofree_string_hash, struct symbol_attributes> dynsym_map;

    /* hash mapping the symbol name (e.g. function name) to attributes in the
       symtab table.  This means symbols that needs externalization hacks to
       be accesses/called.  */
    hash_map<nofree_string_hash, struct symbol_attributes> symtab_map;

    /* Flag to identify if the maps can decide visibility.  */
    bool can_decide_visibility_p;

    /* Set of nodes that we analyzed.  This avoids recursion when doing DFS
       in the graph.  */
    hash_set<symtab_node *> analyzed_nodes;

    /* Set of functions that got modified.  */
    hash_set<struct function *> modified_functions;
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
      // Make sure the global extract and externalize vectors are initialized.
      init_symbols_to_extract();
      init_symbols_to_externalize();
      init_symbols_to_weakly_externalize();

      return !flag_ltrans &&
	     (gsymbols_to_extract.length() | gsymbols_to_externalize.length() |
	      gsymbols_to_weakly_externalize.length());
    }
  unsigned int execute (function *) final override
    {
#if 0
      FILE *outp = fopen ("/tmp/symtab.dot", "w");
      gcc_assert (outp);
      symtab->dump_graphviz(outp);
      fclose (outp);
#endif

      ipa_livepatch_engine lp;
      return lp.execute();
    }

}; // class pass_ipa_livepatch_closure

} // anon namespace

ipa_opt_pass_d *
make_pass_ipa_livepatch_closure (gcc::context *ctxt)
{
  return new pass_ipa_livepatch_closure (ctxt);
}
