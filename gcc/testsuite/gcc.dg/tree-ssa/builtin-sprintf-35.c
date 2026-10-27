/* Verify which sprintf calls with integer directives whose arguments are
   not known at compile time are folded into calls to the __itoa and __utoa
   functions emitted by the sprintf pass, and which are left alone.
   { dg-do compile }
   { dg-options "-O2 -Wno-format -fno-printf-return-value -fdump-tree-optimized" } */

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)

/* Calls expected to be folded.  Signed directives use __itoa and unsigned
   ones __utoa.  */

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
  sprintf (d, "%d:%s:%u", i, s, u);
}

int f_mixed_ret (char *d, int i, unsigned u)
{
  return sprintf (d, "<%d|%u>", i, u);
}

void f_d_d (char *d, int i, int j)
{
  sprintf (d, "%d%d", i, j);
}

/* Calls expected not to be folded because we don't support these formats
   so far.  */

void g_x (char *d, int i)
{
  sprintf (d, "%x", i);
}

void g_o (char *d, int i)
{
  sprintf (d, "%o", i);
}

void g_flag_plus (char *d, int i)
{
  sprintf (d, "%+d", i);
}

void g_flag_zero (char *d, int i)
{
  sprintf (d, "%05d", i);
}

void g_width (char *d, int i)
{
  sprintf (d, "%5d", i);
}

void g_width_star (char *d, int w, int i)
{
  sprintf (d, "%*d", w, i);
}

void g_int_prec (char *d, int i)
{
  sprintf (d, "%.3d", i);
}

void g_uint_prec (char *d, unsigned u)
{
  sprintf (d, "%.3u", u);
}

/* One unsupported directive prevents folding the whole call.  */

void g_mixed (char *d, int i, int j)
{
  sprintf (d, "%d%x", i, j);
}

/* __itoa and __utoa are only emitted on targets supporting COMDAT groups,
   otherwise the f_* functions are not folded.
   { dg-final { scan-tree-dump-times "sprintf \\(" 9 "optimized" { target comdat_group } } }
   { dg-final { scan-tree-dump-times "sprintf \\(" 24 "optimized" { target { ! comdat_group } } } } */
