/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=g -fexternalize-symbols=f" } */

#define MACRO int

MACRO f();

int g()
{
  return f();
}

/* { dg-final { scan-tree-dump "klpe_f" "optimized" } } */
/* { dg-final { scan-tree-dump "_\[0-9\]\+ = klpe_f" "optimized" } } */
/* { dg-final { scan-assembler "klpe_f:" } } */
