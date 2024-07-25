/* { dg-require-effective-target powerpc_elfv2 } */
/* There is no global entry point prologue with pcrel.  */
/* { dg-options "-mno-pcrel -fpatchable-function-entry=1,1" } */

/* Verify there is no error with 1 nop before local entry.  */

extern int a;

int test (int b) {
  return a + b;
}
