/* Verify that sprintf calls with integer directives whose arguments are not
   known at compile time are not folded into calls to the itoa and utoa
   functions with an execution character set whose digits or minus sign
   differ from those of the host character set.  The C library determines
   how such calls are formatted.  */

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

/* { dg-final { scan-tree-dump-times "sprintf \\(" 2 "optimized" } } */
/* { dg-final { scan-assembler-not "toa\[.$\].*gcc_strlen" } } */
