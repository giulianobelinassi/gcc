/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=f -fexternalize-symbols=bbb" } */

int bbb;

#define A bbb
#define B bbb

int f()
{
  return A + B ;
}

/* { dg-final { scan-tree-dump "int f \\(\\)" "optimized" } } */
/* { dg-final { scan-tree-dump "_\[0-9\]\+ = \\*klpe_bbb" "optimized" } } */
/* { dg-final { scan-assembler "klpe_bbb:" } } */
