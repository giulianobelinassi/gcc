/* Declarations of tree-ssa-strlen API.

   Copyright (C) 2018-2026 Free Software Foundation, Inc.

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

#ifndef GCC_TREE_SSA_STRLEN_H
#define GCC_TREE_SSA_STRLEN_H

class pointer_query;

enum strlen_range_kind {
  /* Compute the exact constant string length.  */
  SRK_STRLEN,
  /* Compute the maximum constant string length.  */
  SRK_STRLENMAX,
  /* Compute a range of string lengths bounded by object sizes.  When
     the length of a string cannot be determined, consider as the upper
     bound the size of the enclosing object the string may be a member
     or element of.  Also determine the size of the largest character
     array the string may refer to.  */
  SRK_LENRANGE,
  /* Determine the integer value of the argument (not string length).  */
  SRK_INT_VALUE
};

extern bool is_strlen_related_p (tree, tree);
extern bool maybe_diag_stxncpy_trunc (gimple_stmt_iterator, tree, tree,
				      pointer_query * = NULL);
extern tree set_strlen_range (tree, wide_int, wide_int, tree = NULL_TREE);

extern tree get_range (tree, gimple *, wide_int[2],
		       class range_query * = NULL);

struct c_strlen_data;
extern void get_range_strlen_dynamic (tree, gimple *, c_strlen_data *,
				      pointer_query &);

extern gimple *use_in_zero_equality (tree, bool = true);

/* APIs internal to strlen pass.  Defined in gimple-ssa-sprintf.cc.  */
extern bool handle_printf_call (gimple_stmt_iterator *, pointer_query &);

extern tree get_maxval_strlen (tree, strlen_range_kind, tree * = NULL);

#endif   // GCC_TREE_SSA_STRLEN_H
