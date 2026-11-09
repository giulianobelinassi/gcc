/* Verify that sprintf calls with integer directives whose arguments are
   constant are not folded with an execution character set whose digits or
   minus sign differ from those of the host character set.  The C library
   determines how such calls are formatted.  */

/* { dg-do compile } */
/* { dg-require-iconv "IBM1047" } */
/* { dg-options "-O2 -fexec-charset=IBM1047 -fno-printf-return-value -fdump-tree-optimized" } */

void
f_d (char *d)
{
  __builtin_sprintf (d, "x=%d", -42);
}

void
f_u (char *d)
{
  __builtin_sprintf (d, "<%u>", 7u);
}

/* { dg-final { scan-tree-dump-times "sprintf \\(" 2 "optimized" } } */
