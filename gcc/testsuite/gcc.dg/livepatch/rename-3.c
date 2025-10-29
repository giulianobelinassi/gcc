/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=g -fexternalize-symbols=f" } */

int (*f)(void);

int g(void)
{
  return f();
}

/* { dg-final { scan-tree-dump "int g \\(\\)" "optimized" } } */
/* { dg-final { scan-tree-dump "klpe_f" "optimized"} } */
/* { dg-final { scan-assembler "klpe_f:" } } */
