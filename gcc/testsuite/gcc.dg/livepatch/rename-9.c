/* { dg-do compile } */
/* { dg-options "-fdump-tree-optimized -O2 -fextract-symbols=f -fexternalize-symbols=CRYPTO_strdup" } */

char CRYPTO_strdup(const char *, const char *, int);

#define OPENSSL_strdup(str) \
        CRYPTO_strdup(str, "aaaaa", __LINE__)

int f()
{
  return OPENSSL_strdup("aaa");
}

/* { dg-final { scan-tree-dump "int f \\(\\)" "optimized" } } */
/* { dg-final { scan-tree-dump "klpe_CRYPTO_strdup" "optimized" } } */
/* { dg-final { scan-assembler "klpe_CRYPTO_strdup:" } } */
