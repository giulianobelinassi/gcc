/* Verify that sprintf calls with integer directives whose arguments are not
   known at compile time are folded into calls to the itoa and utoa
   functions with a non-default execution character set, and that the name
   of the character set is part of the names of the functions.  The
   functions are shared with other translation units, which may have been
   compiled with a different execution character set, so only copies using
   the same character set may be merged.  */

/* { dg-do compile } */
/* { dg-require-iconv "IBM1047" } */
/* { dg-require-weak "" } */
/* { dg-options "-O2 -fexec-charset=IBM1047 -fno-printf-return-value -fdump-tree-optimized" } */

void f_d (char *d, int i)
{
  __builtin_sprintf (d, "%d", i);
}

void f_u (char *d, unsigned u)
{
  __builtin_sprintf (d, "%u", u);
}

/* { dg-final { scan-tree-dump-not "sprintf \\(" "optimized" } } */
/* { dg-final { scan-assembler "itoa\[.$\]IBM1047\[.$\]gcc_strlen" } } */
/* { dg-final { scan-assembler "utoa\[.$\]IBM1047\[.$\]gcc_strlen" } } */
/* { dg-final { scan-assembler-not "toa\[.$\]gcc_strlen" } } */
