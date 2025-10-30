/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O0 -fextract-symbols=f -fexternalize-symbols=h,g" } */

int g(int);

int h(int x)
{
  return g(x);
}

int g(int x)
{
  return h(x);
}

int f(void)
{
  return g(3);
}

/* { dg-final { scan-tree-dump "klpe_g" "optimized" } } */
/* { dg-final { scan-tree-dump "klpe_h" "optimized" } } */
/* { dg-final { scan-assembler "klpe_g:" "optimized" } } */
/* { dg-final { scan-assembler "klpe_h:" "optimized" } } */
