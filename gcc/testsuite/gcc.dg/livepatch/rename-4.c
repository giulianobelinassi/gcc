/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=g -fexternalize-symbols=f" } */

__attribute__((noinline)) __attribute__((noipa)) int f();

int f()
{
  return 3;
}

int g()
{
  return f();
}

/* { dg-final { scan-tree-dump "int g \\(\\)" "optimized" } } */
/* { dg-final { scan-tree-dump "klpe_f" "optimized" } } */
/* { dg-final { scan-assembler "klpe_f:" } } */
