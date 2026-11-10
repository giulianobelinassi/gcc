/* Verify that sprintf calls with more than 5 directives, counting the plain
   parts of the format string and its terminating nul, are not folded.
   { dg-do compile }
   { dg-options "-O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

/* Calls with 5 directives are folded.  */

void f_5 (char *d, const char *s, const char *t)
{
  sprintf (d, "<%s|%s", s, t);
}

void f_5_plain (char *d, const char *s, const char *t)
{
  sprintf (d, "%s|%s>", s, t);
}

void f_5_d (char *d)
{
  sprintf (d, "%d%d%d%d", 1, 2, 3, 4);
}

/* Calls with 6 directives are not.  */

void g_6 (char *d, const char *s, const char *t)
{
  sprintf (d, "<%s|%s>", s, t);
}

void g_6_d (char *d)
{
  sprintf (d, "%d%d%d%d%d", 1, 2, 3, 4, 5);
}

/* { dg-final { scan-tree-dump-times "sprintf \\(" 2 "optimized" } } */
