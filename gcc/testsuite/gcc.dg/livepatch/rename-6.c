/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=f,g -fexternalize-symbols=a" } */

static int a;

void f(int x)
{
  a = x;
}

int g()
{
  return a;
}

/* { dg-final { scan-tree-dump "void f \\(int x\\)" "optimized" } } */
/* { dg-final { scan-tree-dump "_\[0-9\]\+ = klpe_a" "optimized" } } */
/* { dg-final { scan-tree-dump "int g \\(\\)" "optimized" } } */
/* { dg-final { scan-tree-dump "_\[0-9\] = *klpe_a" "optimized" } } */
/* { dg-final { scan-tree-dump "klpe_a" "optimized" } } */
/* { dg-final { scan-assembler "klpe_a:" } } */
