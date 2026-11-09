/* Verify that sprintf calls with integer directives whose arguments are
   not known at compile time are folded into calls to the itoa and utoa
   functions emitted by the sprintf optimizations.  Every translation unit
   emits its own copy of these functions, so they are only emitted on targets
   that can merge the copies at link time using weak symbols (such targets also
   often support COMDAT which the sprintf optimizations also can take advantage
   of).  Calls left alone on all targets are in builtin-sprintf-32.c.
   { dg-do compile }
   { dg-require-weak "" }
   { dg-options "-O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

/* Calls expected to be folded.  Signed directives use itoa and unsigned
   ones utoa.  */

void f_d (char *d, int i)
{
  sprintf (d, "%d", i);
}

int f_d_ret (char *d, int i)
{
  return sprintf (d, "%d", i);
}

void f_i (char *d, int i)
{
  sprintf (d, "%i", i);
}

void f_hhd (char *d, int i)
{
  sprintf (d, "%hhd", i);
}

void f_hd (char *d, int i)
{
  sprintf (d, "%hd", i);
}

void f_ld (char *d, long i)
{
  sprintf (d, "%ld", i);
}

void f_u (char *d, unsigned u)
{
  sprintf (d, "%u", u);
}

int f_u_ret (char *d, unsigned u)
{
  return sprintf (d, "%u", u);
}

void f_hhu (char *d, int u)
{
  sprintf (d, "%hhu", u);
}

void f_hu (char *d, int u)
{
  sprintf (d, "%hu", u);
}

void f_lu (char *d, unsigned long u)
{
  sprintf (d, "%lu", u);
}

void f_zu (char *d, __SIZE_TYPE__ u)
{
  sprintf (d, "%zu", u);
}

void f_mixed (char *d, const char *s, int i, unsigned u)
{
  sprintf (d, "%d:%s%u", i, s, u);
}

int f_mixed_ret (char *d, int i, unsigned u)
{
  return sprintf (d, "<%d|%u", i, u);
}

void f_s_d (char *d, const char *s, int i)
{
  sprintf (d, "%s%d", s, i);
}

void f_d_d (char *d, int i, int j)
{
  sprintf (d, "%d%d", i, j);
}

/* On LLP64 targets, size_t is wider than long, which the itoa and utoa
   functions take, so the %zu call is not folded.  */
/* { dg-final { scan-tree-dump-not "sprintf \\(" "optimized" { target { ! llp64 } } } } */
/* { dg-final { scan-tree-dump-times "sprintf \\(" 1 "optimized" { target llp64 } } } */
