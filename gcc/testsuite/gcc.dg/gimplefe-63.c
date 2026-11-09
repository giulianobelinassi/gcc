/* { dg-do compile } */
/* { dg-options "-O2 -fgimple -fno-printf-return-value -fdump-tree-strlen1" } */

/* When folding a sprintf call whose return value is stored directly into
   memory, the length must be converted to int in a register before being
   stored.  Converting it as part of the store produced invalid GIMPLE
   (a conversion with a non-register LHS) and broken virtual operands.
   The C gimplifier always stores the call result into a temporary, so
   such calls can only be written using the GIMPLE FE.  */

int g;
struct S { int a; } s;

/* %s directive, folded into a memcpy sequence.  */

void __GIMPLE (ssa)
f_str (char * d, int c)
{
  char * p;

  __BB(2):
  if (c_1(D) != 0)
    goto __BB3;
  else
    goto __BB4;

  __BB(3):
  goto __BB4;

  __BB(4):
  p_2 = __PHI (__BB3: _Literal (char *) &"ab"[0],
               __BB2: _Literal (char *) &"abcd"[0]);
  g = __builtin_sprintf (d_3(D), _Literal (const char *) &"%s"[0], p_2);
  return;
}

/* %s directive with a string literal argument, stored into a struct
   member.  */

void __GIMPLE (ssa)
f_lit (char * d)
{
  __BB(2):
  s.a = __builtin_sprintf (d_1(D), _Literal (const char *) &"%s"[0],
                           _Literal (const char *) &"abc"[0]);
  return;
}

/* { dg-final { scan-tree-dump-not "__builtin_sprintf \\(" "strlen1" } } */
