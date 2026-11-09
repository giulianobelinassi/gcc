/* Verify that sprintf calls with a %.Ns directive whose argument has
   a length not known at compile time are not folded into a sequence that
   calls strnlen when the program doesn't use strnlen.  strnlen is not an
   ISO C function so the C library may not provide it.
   { dg-do compile }
   { dg-options "-O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

void f_s_prec (char *d, const char *s)
{
  sprintf (d, "%.3s", s);
}

void f_s_prec_plain (char *d, const char *s)
{
  sprintf (d, "<%.3s>", s);
}

/* Calls that don't need strnlen are still folded.  */

void f_s (char *d, const char *s)
{
  sprintf (d, "%s", s);
}

void f_s_prec_const (char *d)
{
  sprintf (d, "%.3s", "hello");
}

/* { dg-final { scan-tree-dump-not "strnlen" "optimized" } } */
/* { dg-final { scan-tree-dump-times "sprintf \\(" 2 "optimized" } } */
