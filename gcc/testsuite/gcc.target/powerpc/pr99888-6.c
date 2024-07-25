/* { dg-require-effective-target powerpc_elfv2 } */
/* There is no global entry point prologue with pcrel.  */
/* { dg-options "-mno-pcrel" } */

/* Verify no error emitted for 4 nops before local entry.  */

extern int a;

__attribute__ ((patchable_function_entry (20, 4)))
int test (int b) {
  return a + b;
}
