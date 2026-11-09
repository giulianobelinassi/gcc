/* Verify that sprintf calls with integer directives whose arguments have
   a non-integer type, or an integer type that can't be converted at this
   point, are not folded and don't cause an ICE.
   { dg-do compile }
   { dg-options "-std=gnu23 -O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

struct S { int i; };

void f_dbl (char *d, double x)
{
  sprintf (d, "%d", x);
}

void f_dbl_cst (char *d)
{
  sprintf (d, "%u", 1.0);
}

void f_struct (char *d, struct S s)
{
  sprintf (d, "%d", s);
}

void f_ptr (char *d, void *p)
{
  sprintf (d, "%ld", p);
}

void f_ptr_int (char *d, void *p)
{
  sprintf (d, "%d", p);
}

#if __BITINT_MAXWIDTH__ >= 575
void f_bitint (char *d, _BitInt(256) b)
{
  sprintf (d, "%d", b);
}

void f_bitint_u (char *d, unsigned _BitInt(135) b)
{
  sprintf (d, "%u", b);
}
#endif

/* { dg-final { scan-tree-dump-times "sprintf \\(" 5 "optimized" { target { ! bitint575 } } } } */
/* { dg-final { scan-tree-dump-times "sprintf \\(" 7 "optimized" { target bitint575 } } } */
