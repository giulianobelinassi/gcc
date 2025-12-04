/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=main -fexternalize-symbols=func1 -g0" }*/

struct AA {
  void *fun;
};

int func1(void)
{
  return 0;
}

struct AA A = {
  .fun = func1, // { dg-warning "Unable to fully externalize func1" }
};

int main(void)
{
  return (int) ((unsigned long)A.fun & 0xFFFFFFFF);
}

/* { dg-final { scan-assembler-not "func1" } } */
