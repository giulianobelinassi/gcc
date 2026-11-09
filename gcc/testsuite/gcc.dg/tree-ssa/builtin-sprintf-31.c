/* Same as builtin-sprintf-30.c but with -fprintf-return-value enabled, so
   that calls whose return value is known are folded into memcpy without
   a left hand side.
   { dg-do run }
   { dg-options "-O2 -Wno-format -Wno-format-zero-length -fprintf-return-value" } */

#include "builtin-sprintf-30.c"
