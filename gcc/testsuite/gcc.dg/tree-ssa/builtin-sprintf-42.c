/* Verify that when optimizing for size, only sprintf calls with no
   directives other than plain strings or with a single %s directive are
   folded.
   { dg-do compile }
   { dg-options "-Os -Wno-format -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

/* Calls expected to be folded.  */

void f_plain (char *d)
{
  sprintf (d, "hello");
}

void f_s (char *d, const char *s)
{
  sprintf (d, "%s", s);
}

/* Calls expected not to be folded.  */

void f_s_plain (char *d, const char *s)
{
  sprintf (d, "<%s>", s);
}

void g_s_s (char *d, const char *s, const char *t)
{
  sprintf (d, "%s%s", s, t);
}

void g_d (char *d)
{
  sprintf (d, "%d", 1);
}

void g_d_var (char *d, int i)
{
  sprintf (d, "%d", i);
}

void g_s_d (char *d, const char *s, int i)
{
  sprintf (d, "%s%d", s, i);
}

/* { dg-final { scan-tree-dump-times "sprintf \\(" 5 "optimized" } } */
