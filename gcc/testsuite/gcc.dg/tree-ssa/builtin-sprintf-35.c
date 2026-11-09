/* Verify that sprintf calls with a %.Ns directive whose argument is
   an array that need not be nul-terminated produce the correct output.
   The array need not contain a nul if the precision is no greater than
   the size of the array.
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

char a1[1] = { 'x' };
char a4[4] = { 'a', 'b', 'c', 'd' };
const char ca2[2] = { 'y', 'z' };

struct S { char a[2]; char b[2]; } s = { { 'p', 'q' }, { 'r', 's' } };

__attribute__ ((noipa)) void
f_a1 (char *d)
{
  __builtin_sprintf (d, "%.1s", a1);
}

__attribute__ ((noipa)) void
f_a4_tail (char *d)
{
  __builtin_sprintf (d, "%.1s", &a4[3]);
}

__attribute__ ((noipa)) void
f_a4 (char *d)
{
  __builtin_sprintf (d, "<%.4s>", a4);
}

__attribute__ ((noipa)) void
f_ca2 (char *d)
{
  __builtin_sprintf (d, "%.2s", ca2);
}

__attribute__ ((noipa)) void
f_member (char *d)
{
  __builtin_sprintf (d, "%.2s", s.a);
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

  f_a1 (buf);
  check (buf, "x");
  f_a4_tail (buf);
  check (buf, "d");
  f_a4 (buf);
  check (buf, "<abcd>");
  f_ca2 (buf);
  check (buf, "yz");
  f_member (buf);
  check (buf, "pq");

  return 0;
}
