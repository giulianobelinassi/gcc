/* Verify that sprintf calls with a %.Ns directive whose argument is an array
   with a nul character followed by other characters, and with a precision
   greater than the length of the string, are folded and write only the
   characters before the nul.
   { dg-do run }
   { dg-options "-O2 -Wno-format -fdump-tree-optimized" } */

typedef __SIZE_TYPE__ size_t;

/* Make strnlen available for the sprintf optimizations.  */
size_t strnlen (const char *, size_t);

__attribute__ ((noipa)) size_t
use_strnlen (const char *s)
{
  return strnlen (s, 4);
}

/* The contents are known at compile time.  */
static const char cst[8] = "ab\0cdefg";

/* The contents are not known at compile time.  */
char var[8] = { 'a', 'b', '\0', 'c', 'd', 'e', 'f', 'g' };

__attribute__ ((noipa)) int
f_cst (char *d)
{
  return __builtin_sprintf (d, "%.5s", cst);
}

__attribute__ ((noipa)) int
f_cst_plain (char *d)
{
  return __builtin_sprintf (d, "<%.7s>", cst);
}

/* The nul is the last character within the precision.  */

__attribute__ ((noipa)) int
f_cst_prec_3 (char *d)
{
  return __builtin_sprintf (d, "%.3s", cst);
}

/* Past the nul, the string is "cdefg" without a terminating nul, so the
   precision must be no greater than its size.  */

__attribute__ ((noipa)) int
f_cst_after_nul (char *d)
{
  return __builtin_sprintf (d, "%.5s", cst + 3);
}

__attribute__ ((noipa)) int
f_var (char *d)
{
  return __builtin_sprintf (d, "%.5s", var);
}

__attribute__ ((noipa)) int
f_var_plain (char *d)
{
  return __builtin_sprintf (d, "<%.7s>", var);
}

__attribute__ ((noipa)) int
f_var_ptr (char *d, const char *s)
{
  return __builtin_sprintf (d, "%.6s", s);
}

/* Check that BUF holds the string EXPECT and that nothing after its
   terminating nul has been overwritten.  */

static void
check (const char *buf, int ret, const char *expect)
{
  size_t len = __builtin_strlen (expect);
  if (ret != (int) len)
    __builtin_abort ();
  if (__builtin_memcmp (buf, expect, len + 1) != 0)
    __builtin_abort ();
  for (size_t i = len + 1; i < 16; i++)
    if (buf[i] != 'X')
      __builtin_abort ();
}

#define TEST(call, expect)			\
  do						\
    {						\
      __builtin_memset (buf, 'X', sizeof buf);	\
      int ret = call;				\
      check (buf, ret, expect);			\
    }						\
  while (0)

int
main (void)
{
  char buf[16];

  if (use_strnlen ("ab") != 2)
    __builtin_abort ();

  TEST (f_cst (buf), "ab");
  TEST (f_cst_plain (buf), "<ab>");
  TEST (f_cst_prec_3 (buf), "ab");
  TEST (f_cst_after_nul (buf), "cdefg");
  TEST (f_var (buf), "ab");
  TEST (f_var_plain (buf), "<ab>");
  TEST (f_var_ptr (buf, var), "ab");
  TEST (f_var_ptr (buf, var + 2), "");
  TEST (f_var_ptr (buf, var + 3), "cdefg");

  return 0;
}

/* All calls are folded.  */
/* { dg-final { scan-tree-dump-not "sprintf \\(" "optimized" } } */
