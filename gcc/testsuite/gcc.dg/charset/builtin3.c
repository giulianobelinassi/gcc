/* Ensure that sprintf calls with integer directives folded by the sprintf
   optimizations produce output in the execution character set.  */

/* { dg-do run } */
/* { dg-require-iconv "IBM1047" } */
/* { dg-options "-O2 -fexec-charset=IBM1047 -fno-printf-return-value -fdump-tree-optimized" } */
/* { dg-final { scan-tree-dump-not "sprintf" "optimized" } } */

/* "x=-42" and "<7>" in IBM1047.  */
static const unsigned char expect_d[] = { 0xa7, 0x7e, 0x60, 0xf4, 0xf2, 0 };
static const unsigned char expect_u[] = { 0x4c, 0xf7, 0x6e, 0 };

__attribute__ ((noipa)) void
f_d (char *d)
{
  __builtin_sprintf (d, "x=%d", -42);
}

__attribute__ ((noipa)) void
f_u (char *d)
{
  __builtin_sprintf (d, "<%u>", 7u);
}

int
main (void)
{
  char buf[16];

  f_d (buf);
  if (__builtin_memcmp (buf, expect_d, sizeof expect_d) != 0)
    __builtin_abort ();

  f_u (buf);
  if (__builtin_memcmp (buf, expect_u, sizeof expect_u) != 0)
    __builtin_abort ();

  return 0;
}
