/* { dg-require-effective-target powerpc_elfv2 } */
/* There is no global entry point prologue with pcrel.  */
/* { dg-options "-mno-pcrel -fpatchable-function-entry=7,3" } */

/* Verify no error emitted for 3 nops before local entry.  */

extern int a;

int test (int b) {
  return a + b;
}
