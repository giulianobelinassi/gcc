/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=g -fexternalize-symbols=f" } */

int f(int x)
{
  return x;
}

struct AAA
{
  int a;
};

struct AAA* g(int x)
{
  static struct AAA aa;
  aa.a = x;
  return &aa;
}

/* { dg-final { scan-tree-dump "struct AAA \\* g" "optimized" } } */
/* { dg-final { scan-tree-dump-not "klpe_f" "optimized" } } */
