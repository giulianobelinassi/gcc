/* { dg-options "-fdump-tree-optimized -fextract-symbols=f -fexternalize-symbols=g -O2" }*/
__attribute__((noinline)) static int g()
{
  volatile int x = 3;
  return x;
}

int f(void)
{
  return g();
}

/* { dg-final { scan-tree-dump-not "int g" "optimized" } } */
/* { dg-final { scan-tree-dump "klpe_g" "optimized" } } */
/* { dg-final { scan-assembler "klpe_g:" } } */
