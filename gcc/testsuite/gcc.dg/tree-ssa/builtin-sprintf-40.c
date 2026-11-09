/* Verify that the itoa and utoa helpers aren't emitted when a sprintf call
   using them can't be folded because of some other directive in it.
   { dg-do compile }
   { dg-options "-O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

void f_d_x (char *d, int i, int x)
{
  sprintf (d, "%d %x", i, x);
}

void f_u_percent (char *d, unsigned u)
{
  sprintf (d, "%u%%", u);
}

/* { dg-final { scan-tree-dump-times "sprintf \\(" 2 "optimized" } } */
/* { dg-final { scan-assembler-not "toa\[.$\].*gcc_strlen" } } */
