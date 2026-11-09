/* Verify that sprintf calls with a %.*s directive whose precision is known
   to be negative are folded as if the precision were omitted.
   { dg-do compile }
   { dg-options "-O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

void f_neg (char *d, const char *s)
{
  sprintf (d, "%.*s", -1, s);
}

void f_neg_const (char *d)
{
  sprintf (d, "%.*s", -5, "hello");
}

void f_neg_range (char *d, int p, const char *s)
{
  if (p < 0)
    sprintf (d, "<%.*s>", p, s);
}

void f_neg_range_const (char *d, int p)
{
  if (p >= -3 && p <= -1)
    sprintf (d, "%.*s", p, "hello");
}

/* A precision that may be either negative or not is not folded.  */

void g_mixed_range (char *d, int p, const char *s)
{
  if (p >= -3 && p <= 3)
    sprintf (d, "%.*s", p, s);
}

/* { dg-final { scan-tree-dump-not "strnlen" "optimized" } } */
/* { dg-final { scan-tree-dump-times "sprintf \\(" 1 "optimized" } } */
