/* Verify that sprintf calls with integer directives whose arguments are not
   known at compile time are folded into calls to the itoa and utoa
   functions with a non-default execution character set, and that the
   output is in that character set.  The functions take the digits and the
   sign in the execution character set as an argument, so their copies
   don't depend on it and have the same names as with the default one.  */

/* { dg-do run } */
/* { dg-require-iconv "IBM1047" } */
/* { dg-require-weak "" } */
/* { dg-options "-O2 -fexec-charset=IBM1047 -fno-printf-return-value -fdump-tree-optimized" } */

/* "x=-42", "1234567890" and "<-9>" in IBM1047.  */
static const unsigned char expect_d[] = { 0xa7, 0x7e, 0x60, 0xf4, 0xf2, 0 };
static const unsigned char expect_u[] = { 0xf1, 0xf2, 0xf3, 0xf4, 0xf5,
					  0xf6, 0xf7, 0xf8, 0xf9, 0xf0, 0 };
static const unsigned char expect_dd[] = { 0x4c, 0x60, 0xf9, 0x6e, 0 };

__attribute__ ((noipa)) void
f_d (char *d, int i)
{
  __builtin_sprintf (d, "x=%d", i);
}

__attribute__ ((noipa)) void
f_u (char *d, unsigned u)
{
  __builtin_sprintf (d, "%u", u);
}

__attribute__ ((noipa)) void
f_d_plain (char *d, int i)
{
  __builtin_sprintf (d, "<%d>", i);
}

int
main (void)
{
  char buf[16];

  f_d (buf, -42);
  if (__builtin_memcmp (buf, expect_d, sizeof expect_d) != 0)
    __builtin_abort ();

  f_u (buf, 1234567890u);
  if (__builtin_memcmp (buf, expect_u, sizeof expect_u) != 0)
    __builtin_abort ();

  f_d_plain (buf, -9);
  if (__builtin_memcmp (buf, expect_dd, sizeof expect_dd) != 0)
    __builtin_abort ();

  return 0;
}

/* { dg-final { scan-tree-dump-not "sprintf \\(" "optimized" } } */
/* { dg-final { scan-tree-dump-times "= itoa\[.$\]gcc_strlen \\(" 2 "optimized" } } */
/* { dg-final { scan-tree-dump-times "= utoa\[.$\]gcc_strlen \\(" 1 "optimized" } } */
