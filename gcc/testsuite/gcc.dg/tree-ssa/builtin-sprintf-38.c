/* Verify that sprintf calls with a %.*s directive whose precision is not
   known exactly at compile time, but is known to be in some range, produce
   the correct output.
   { dg-do run }
   { dg-options "-O2 -Wno-format" } */

typedef __SIZE_TYPE__ size_t;

/* Make strnlen available for the sprintf optimizations.  */
size_t strnlen (const char *, size_t);

__attribute__ ((noipa)) size_t
use_strnlen (const char *s)
{
  return strnlen (s, 4);
}

const char *volatile vhello = "hello";

__attribute__ ((noipa)) void
prec_1_3 (char *d, int p, const char *s)
{
  if (p >= 1 && p <= 3)
    __builtin_sprintf (d, "%.*s", p, s);
}

__attribute__ ((noipa)) void
prec_1_3_const (char *d, int p)
{
  if (p >= 1 && p <= 3)
    __builtin_sprintf (d, "%.*s", p, "hello");
}

/* A negative precision is taken as if it were omitted.  */

__attribute__ ((noipa)) void
prec_m3_3 (char *d, int p, const char *s)
{
  if (p >= -3 && p <= 3)
    __builtin_sprintf (d, "%.*s", p, s);
}

__attribute__ ((noipa)) void
prec_m3_3_const (char *d, int p)
{
  if (p >= -3 && p <= 3)
    __builtin_sprintf (d, "%.*s", p, "hello");
}

/* A precision known to be negative is also taken as if it were omitted.  */

__attribute__ ((noipa)) void
prec_neg (char *d, const char *s)
{
  __builtin_sprintf (d, "%.*s", -1, s);
}

__attribute__ ((noipa)) void
prec_m3_m1 (char *d, int p, const char *s)
{
  if (p >= -3 && p <= -1)
    __builtin_sprintf (d, "%.*s", p, s);
}

__attribute__ ((noipa)) void
prec_m3_m1_const (char *d, int p)
{
  if (p >= -3 && p <= -1)
    __builtin_sprintf (d, "<%.*s>", p, "hello");
}

static void
check (const char *buf, const char *expect)
{
  if (__builtin_strcmp (buf, expect) != 0)
    __builtin_abort ();
}

int
main (void)
{
  char buf[16];

  if (use_strnlen ("ab") != 2)
    __builtin_abort ();

  prec_1_3 (buf, 1, vhello);
  check (buf, "h");
  prec_1_3 (buf, 2, vhello);
  check (buf, "he");
  prec_1_3 (buf, 3, vhello);
  check (buf, "hel");

  prec_1_3_const (buf, 1);
  check (buf, "h");
  prec_1_3_const (buf, 3);
  check (buf, "hel");

  prec_m3_3 (buf, -1, vhello);
  check (buf, "hello");
  prec_m3_3 (buf, 2, vhello);
  check (buf, "he");

  prec_m3_3_const (buf, -1);
  check (buf, "hello");
  prec_m3_3_const (buf, 2);
  check (buf, "he");

  prec_neg (buf, vhello);
  check (buf, "hello");

  prec_m3_m1 (buf, -1, vhello);
  check (buf, "hello");
  prec_m3_m1 (buf, -3, vhello);
  check (buf, "hello");

  prec_m3_m1_const (buf, -2);
  check (buf, "<hello>");

  return 0;
}
