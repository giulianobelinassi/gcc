/* { dg-do compile } */
/* { dg-with-debuginfo "" } */
/* { dg-options "-fno-lto -fdump-tree-optimized -O2 -fextract-symbols=f -fweakly-externalize-symbols=strdup_new" } */

unsigned long strlen (const char *);
void *malloc (unsigned long);
void *memcpy (void *, const void *, unsigned long);

__attribute__((noinline))
char *strdup_new(const char *str)
{
  unsigned size = strlen (str) + 1;
  char *ret = malloc (size);
  memcpy (ret, str, size);

  return ret;
}

char *f(void)
{
  return strdup_new("aaa");
}

/* { dg-final { scan-assembler "f:" } } */
/* { dg-final { scan-assembler-not "strdup_new:" } } */
