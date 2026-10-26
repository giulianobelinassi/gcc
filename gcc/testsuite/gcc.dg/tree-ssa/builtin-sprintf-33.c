/* Verify that sprintf calls with integer directives whose arguments are not
   known at compile time, which are folded into calls to the __itoa and
   __utoa functions emitted by the sprintf pass, produce the same output and
   the same return value as the library sprintf.  -fno-printf-return-value
   prevents the return value from being replaced by a constant computed by
   the sprintf pass so that the value computed by the folded sequence is the
   one being checked.
   { dg-do run }
   { dg-options "-O2 -Wno-format -fno-printf-return-value" } */

typedef __SIZE_TYPE__ size_t;

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)
#define putchar(...) __builtin_putchar (__VA_ARGS__)
#define puts(...) __builtin_puts (__VA_ARGS__)

/* Larger than any output below.  */
#define BUFSIZE 80

const char *volatile vhello = "hello";

unsigned nfails;

/* Return the argument in a way the compiler cannot see through, so that the
   sprintf calls below are not folded as if it were a constant.  */
#define IDENT(name, type)				\
  static type __attribute__ ((noipa))			\
  name (type x)						\
  {							\
    return x;						\
  }

IDENT (vi, int)
IDENT (vu, unsigned)
IDENT (vl, long)
IDENT (vul, unsigned long)
IDENT (vll, long long)
IDENT (vull, unsigned long long)
IDENT (vz, size_t)
IDENT (vt, __PTRDIFF_TYPE__)
IDENT (vj, __INTMAX_TYPE__)
IDENT (vuj, __UINTMAX_TYPE__)

static void __attribute__ ((noipa))
print_buffer (int n, const char *buf)
{
  for (int i = 0; i < n; ++i)
    if (buf[i] == '\0')
      putchar('$');
    else
      putchar(buf[i]);
  putchar('\n');
}

/* Check that BUF contains the string EXPECT and that the byte past its
   terminating nul has not been clobbered.  Unless N is negative, also
   check that N, the value returned by sprintf, equals the length of
   EXPECT.  */

static void __attribute__ ((noipa))
check (int line, const char *buf, int n, const char *expect)
{
  size_t len = __builtin_strlen (expect);

  if ((n >= 0 && (size_t) n != len)
      || __builtin_memcmp (buf, expect, len + 1) != 0
      || buf[len + 1] != 'X')
    {
      __builtin_printf ("line %i: sprintf returned %i, wrote \"%s\"; "
			"expected %i, \"%s\"\n",
			line, n, buf, (int) len, expect);
      puts("Written buffer: ");
      print_buffer(BUFSIZE, buf);
      ++nfails;
    }
}

/* Call sprintf and check both its output and its return value.  */
#define T(expect, ...)						\
  do {								\
    char buf[BUFSIZE];						\
    __builtin_memset (buf, 'X', sizeof buf);			\
    buf[sizeof buf - 1] = '\0';					\
    int n = sprintf (buf, __VA_ARGS__);				\
    check (__LINE__, buf, n, expect);				\
  } while (0)

/* Call sprintf ignoring its return value and check its output.  */
#define V(expect, ...)						\
  do {								\
    char buf[BUFSIZE];						\
    __builtin_memset (buf, 'X', sizeof buf);			\
    buf[sizeof buf - 1] = '\0';					\
    sprintf (buf, __VA_ARGS__);					\
    check (__LINE__, buf, -1, expect);				\
  } while (0)

/* Do both of the above.  */
#define TV(expect, ...)						\
  do {								\
    T (expect, __VA_ARGS__);					\
    V (expect, __VA_ARGS__);					\
  } while (0)

/* Signed directives, handled by __itoa.  */

static void __attribute__ ((noipa))
test_itoa (void)
{
  TV ("0", "%d", vi (0));
  TV ("1", "%d", vi (1));
  TV ("-1", "%d", vi (-1));
  TV ("9", "%d", vi (9));
  TV ("10", "%d", vi (10));
  TV ("-9", "%d", vi (-9));
  TV ("-10", "%d", vi (-10));
  TV ("99", "%i", vi (99));
  TV ("100", "%i", vi (100));
  TV ("-123456", "%i", vi (-123456));

  /* The argument is converted to the type of the directive.  */
  TV ("-128", "%hhd", vi (-128));
  TV ("127", "%hhd", vi (127));
  TV ("-56", "%hhd", vi (200));
  TV ("-32768", "%hd", vi (-32768));
  TV ("-25536", "%hd", vi (40000));

  TV ("-123", "%ld", vl (-123));
  TV ("-123", "%td", vt (-123));
  TV ("-123", "%jd", vj (-123));
  TV ("-123", "%lld", vll (-123));

#if __INT_MAX__ == 2147483647
  TV ("2147483647", "%d", vi (__INT_MAX__));
  TV ("-2147483648", "%d", vi (-__INT_MAX__ - 1));
#endif

#if __LONG_MAX__ == 9223372036854775807L
  TV ("9223372036854775807", "%ld", vl (__LONG_MAX__));
  TV ("-9223372036854775808", "%ld", vl (-__LONG_MAX__ - 1));
#elif __LONG_MAX__ == 2147483647L
  TV ("2147483647", "%ld", vl (__LONG_MAX__));
  TV ("-2147483648", "%ld", vl (-__LONG_MAX__ - 1));
#endif

#if __LONG_LONG_MAX__ == 9223372036854775807LL
  TV ("9223372036854775807", "%lld", vll (__LONG_LONG_MAX__));
  TV ("-9223372036854775808", "%lld", vll (-__LONG_LONG_MAX__ - 1));
#endif
}

/* Unsigned directives, handled by __utoa.  */

static void __attribute__ ((noipa))
test_utoa (void)
{
  TV ("0", "%u", vu (0));
  TV ("1", "%u", vu (1));
  TV ("9", "%u", vu (9));
  TV ("10", "%u", vu (10));
  TV ("123456", "%u", vu (123456));

  /* The argument is converted to the type of the directive.  */
  TV ("255", "%hhu", vi (255));
  TV ("5", "%hhu", vi (261));
  TV ("255", "%hhu", vi (-1));
  TV ("65535", "%hu", vi (65535));
  TV ("4464", "%hu", vi (70000));

  TV ("123", "%lu", vul (123));
  TV ("123", "%zu", vz (123));
  TV ("123", "%ju", vuj (123));
  TV ("123", "%llu", vull (123));

#if __INT_MAX__ == 2147483647
  TV ("2147483647", "%u", vu (__INT_MAX__));
  TV ("2147483648", "%u", vu (__INT_MAX__ + 1U));
  TV ("3000000000", "%u", vu (3000000000U));
  TV ("4294967295", "%u", vu (-1U));
#endif

#if __LONG_MAX__ == 9223372036854775807L
  TV ("9223372036854775808", "%lu", vul (__LONG_MAX__ + 1LU));
  TV ("18446744073709551615", "%lu", vul (-1LU));
#elif __LONG_MAX__ == 2147483647L
  TV ("2147483648", "%lu", vul (__LONG_MAX__ + 1LU));
  TV ("4294967295", "%lu", vul (-1LU));
#endif

#if __LONG_LONG_MAX__ == 9223372036854775807LL
  TV ("18446744073709551615", "%llu", vull (-1LLU));
#endif
}

/* Unknown integers mixed with other directives, so that __itoa and __utoa
   write at an offset into the buffer and their result is used to compute
   where the next directive starts.  */

static void __attribute__ ((noipa))
test_mixed (void)
{
  TV ("x=-7, s=abc, u=7.", "x=%d, s=%s, u=%u.", vi (-7), "abc", vu (7));
  TV ("-7abc", "%d%s", vi (-7), "abc");
  TV ("abc-7", "%s%d", "abc", vi (-7));
  TV ("1 2 3", "%d %i %u", vi (1), vi (2), vu (3));
  TV ("123", "%d%d%d", vi (1), vi (2), vi (3));
  TV ("-1-2-3", "%d%d%d", vi (-1), vi (-2), vi (-3));
  TV ("10-100", "%u%d", vu (10), vi (-100));
  TV ("he42", "%.2s%d", "hello", vi (42));
  TV ("42he", "%d%.2s", vi (42), "hello");
  TV ("[42:-42]", "[%d:%d]", 42, vi (-42));
  TV ("[-42:42]", "[%d:%u]", vi (-42), 42U);

  V ("hello-42", "%s%d", vhello, vi (-42));
  V ("-42hello", "%d%s", vi (-42), vhello);
  V ("-42hel", "%d%.3s", vi (-42), vhello);
  V ("hel42", "%.3s%u", vhello, vu (42));
}

/* Directives the folding does not handle for unknown integers must still
   work.  */

static void __attribute__ ((noipa))
test_unhandled (void)
{
  TV ("ff", "%x", vi (255));
  TV ("17", "%o", vi (15));
  TV ("+5", "%+d", vi (5));
  TV (" 5", "% d", vi (5));
  TV ("00042", "%05d", vi (42));
  TV ("   42", "%5d", vi (42));
  TV ("007", "%.3d", vi (7));
  TV ("007", "%.3u", vu (7));
}

int
main (void)
{
  test_itoa ();
  test_utoa ();
  test_mixed ();
  test_unhandled ();

  /* Return failure rather than calling abort so that the output
     describing the failures is flushed.  */
  if (nfails)
    {
      __builtin_printf ("%u failures\n", nfails);
      return 1;
    }

  return 0;
}
