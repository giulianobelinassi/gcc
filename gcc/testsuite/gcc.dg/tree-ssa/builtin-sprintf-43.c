/* Verify that when optimizing for size, sprintf calls with integer
   directives are not folded, whether their arguments are constant or not,
   while calls with a single %s directive are.
   { dg-do compile }
   { dg-options "-Os -Wno-format -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

/* Calls expected not to be folded.  */

void g_d (char *d)
{
  sprintf (d, "%d", 1);
}

void g_d_var (char *d, int i)
{
  sprintf (d, "%d", i);
}

void g_i_var (char *d, int i)
{
  sprintf (d, "%i", i);
}

void g_u_var (char *d, unsigned u)
{
  sprintf (d, "%u", u);
}

/* Call expected to be folded.  */

void f_s (char *d, const char *s)
{
  sprintf (d, "%s", s);
}

/* { dg-final { scan-tree-dump-times "sprintf \\(" 4 "optimized" } } */
