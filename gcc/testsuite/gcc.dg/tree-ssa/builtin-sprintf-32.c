/* Verify which sprintf calls are folded into sequences of memcpy calls
   and which are left alone.
   { dg-do compile }
   { dg-options "-O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

typedef __SIZE_TYPE__ size_t;

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

/* Calls expected to be folded.  */

void f_plain (char *d)
{
  sprintf (d, "hello");
}

int f_plain_ret (char *d)
{
  return sprintf (d, "hello");
}

void f_s (char *d, const char *s)
{
  sprintf (d, "%s", s);
}

void f_s_plain (char *d, const char *s)
{
  sprintf (d, "<%s>", s);
}

void f_s_s (char *d, const char *s, const char *t)
{
  sprintf (d, "%s%s", s, t);
}

void f_s_prec (char *d, const char *s)
{
  sprintf (d, "%.3s", s);
}

void f_s_prec_const (char *d)
{
  sprintf (d, "%.3s", "hello");
}

int f_s_range_ret (char *d, int c)
{
  return sprintf (d, "%s", c ? "ab" : "abcd");
}

void f_d (char *d)
{
  sprintf (d, "%d", -123);
}

void f_u (char *d)
{
  sprintf (d, "%u", 123u);
}

int f_d_ret (char *d)
{
  return sprintf (d, "%i", 123);
}

void f_mixed (char *d, const char *s)
{
  sprintf (d, "%d:%s:%u", -1, s, 2u);
}

/* Integers not known at compile time are converted by calls to __itoa.  */

void f_d_var (char *d, int i)
{
  sprintf (d, "%d", i);
}

void f_mixed_var (char *d, const char *s, int i)
{
  sprintf (d, "%s%d", s, i);
}

/* { dg-final { scan-tree-dump-times "__builtin_strnlen" 1 "optimized" } } */

/* Calls expected not to be folded because we don't support these formats so
   far.  */

void g_width (char *d, const char *s)
{
  sprintf (d, "%5s", s);
}

void g_width_star (char *d, int w, const char *s)
{
  sprintf (d, "%*s", w, s);
}

void g_prec_star (char *d, int p, const char *s)
{
  sprintf (d, "%.*s", p, s);
}

void g_flag_minus (char *d, const char *s)
{
  sprintf (d, "%-s", s);
}

void g_flag_plus (char *d)
{
  sprintf (d, "%+d", 1);
}

void g_flag_zero (char *d)
{
  sprintf (d, "%05d", 1);
}

void g_int_prec (char *d)
{
  sprintf (d, "%.3d", 1);
}

void g_x (char *d)
{
  sprintf (d, "%x", 255);
}

void g_c (char *d)
{
  sprintf (d, "%c", 'a');
}

void g_percent (char *d)
{
  sprintf (d, "100%%");
}

/* The length of S is unknown and so is the return value.  */

int g_s_ret (char *d, const char *s)
{
  return sprintf (d, "%s", s);
}

/* __itoa and __utoa are only emitted on targets supporting COMDAT groups,
   otherwise f_d_var and f_mixed_var are not folded.
   { dg-final { scan-tree-dump-times "sprintf \\(" 11 "optimized" { target comdat_group } } }
   { dg-final { scan-tree-dump-times "sprintf \\(" 13 "optimized" { target { ! comdat_group } } } } */
