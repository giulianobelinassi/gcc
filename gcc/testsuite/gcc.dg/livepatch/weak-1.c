/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=f -fweakly-externalize-symbols=g" } */

int g(int);
int sink(int);

int h(int x)
{
  int n = 10;
  int i = 0;
  while ((n++) + x < 20) {
    i = sink(i++);
  }
  return i;
}

__attribute__((noinline))
int g(int x)
{
  return h(x);
}

int f(void)
{
  return g(3);
}

/* { dg-final { scan-assembler "f:" } } */
/* { dg-final { scan-assembler "jmp	g" } } */
/* { dg-final { scan-assembler-not "klpe_g" } } */
/* { dg-final { scan-assembler-not "g:" } } */
