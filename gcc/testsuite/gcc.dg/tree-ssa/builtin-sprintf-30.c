/* Verify that sprintf calls broken into sequences of memcpy calls produce
   the same output and the same return value as the library sprintf.
   -fno-printf-return-value prevents the return value from being replaced
   by a constant computed by the sprintf pass so that the value computed
   by the folded sequence is the one being checked.
   { dg-do run }
   { dg-options "-O2 -Wno-format -Wno-format-zero-length -fno-printf-return-value" } */

typedef __SIZE_TYPE__ size_t;

#define sprintf(...) __builtin_sprintf (__VA_ARGS__)
#define putchar(...) __builtin_putchar (__VA_ARGS__)
#define puts(...) __builtin_puts (__VA_ARGS__)

/* Larger than any output below.  */
#define BUFSIZE 80

volatile int vzero = 0;
volatile int vone = 1;

const char *volatile vhello = "hello";
const char *volatile vh = "h";
const char *volatile vempty = "";

unsigned nfails;

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

static void __attribute__ ((noipa))
test_plain (void)
{
  TV ("a", "a");
  TV ("hello", "hello");
  TV ("hello world", "hello world");
  TV ("", "");
}

static void __attribute__ ((noipa))
test_s_const (void)
{
  TV ("", "%s", "");
  TV ("a", "%s", "a");
  TV ("hello", "%s", "hello");
  TV ("x", "%sx", "");
  TV ("x", "x%s", "");
  TV ("<abc>", "<%s>", "abc");
  TV ("abc>", "%s>", "abc");
  TV ("<abc", "<%s", "abc");
  TV ("abcdef", "%s%s", "abc", "def");
  TV ("a-b-c", "%s-%s-%s", "a", "b", "c");
  TV ("abc", "%s%s%s", "", "abc", "");
}

static void __attribute__ ((noipa))
test_s_const_prec (void)
{
  TV ("", "%.0s", "hello");
  TV ("[]", "[%.0s]", "hello");
  TV ("h", "%.1s", "hello");
  TV ("he", "%.2s", "hello");
  TV ("he!", "%.2s!", "hello");
  TV ("!he", "!%.2s", "hello");
  TV ("hello", "%.5s", "hello");
  TV ("hello", "%.6s", "hello");
  TV ("hello", "%.10s", "hello");
  TV ("hello!", "%.10s!", "hello");
  TV ("helworld", "%.3s%s", "hello", "world");
  TV ("hellowor", "%s%.3s", "hello", "world");
  TV ("helwor", "%.3s%.3s", "hello", "world");
  TV ("he-wo", "%.2s-%.2s", "hello", "world");
}

/* Strings whose length is not constant but is bounded, so the return
   value of sprintf is still known to be in range and the call can be
   folded even when its result is used.  */

static void __attribute__ ((noipa))
test_s_range (int c)
{
  const char *s2 = c ? "ab" : "abcd";
  const char *s5 = c ? "ab" : "abcde";

  TV (c ? "ab" : "abcd", "%s", s2);
  TV (c ? "<ab>" : "<abcd>", "<%s>", s2);
  TV (c ? "ab>" : "abcd>", "%s>", s2);
  TV (c ? "<ab" : "<abcd", "<%s", s2);
  TV (c ? "abab" : "abcdabcd", "%s%s", s2, s2);
  TV (c ? "ab-ab" : "abcd-abcd", "%s-%s", s2, s2);

  TV (c ? "ab" : "abc", "%.3s", s5);
  TV (c ? "ab." : "abc.", "%.3s.", s5);
  TV (c ? ".ab" : ".abc", ".%.3s", s5);
  TV (c ? "abab" : "abcabcde", "%.3s%s", s5, s5);
  TV (c ? "abab" : "abcdeabc", "%s%.3s", s5, s5);
  TV ("", "%.0s", s5);
  TV ("[]", "[%.0s]", s5);
  TV (c ? "ab" : "abcde", "%.5s", s5);
  TV (c ? "ab" : "abcde", "%.9s", s5);
}

/* Strings whose length is not known at all.  The return value of these
   calls cannot be folded, so only check the output.  */

static void __attribute__ ((noipa))
test_s_unknown (void)
{
  V ("hello", "%s", vhello);
  V ("", "%s", vempty);
  V ("[hello]", "[%s]", vhello);
  V ("hellohello", "%s%s", vhello, vhello);

  V ("hel", "%.3s", vhello);
  V ("h", "%.3s", vh);
  V ("", "%.3s", vempty);
  V ("", "%.0s", vhello);
  V ("[hel]", "[%.3s]", vhello);
  V ("hel!", "%.3s!", vhello);
  V ("helhello", "%.3s%s", vhello, vhello);
  V ("hellohel", "%s%.3s", vhello, vhello);
  V ("hhello", "%.3s%s", vh, vhello);

  /* The result is used but the call must still produce correct output.  */
  T ("hello", "%s", vhello);
  T ("hel", "%.3s", vhello);
}

/* Directives the folding does not handle must still work.  */

static void __attribute__ ((noipa))
test_unhandled (int i, int w)
{
  TV ("%", "%%");
  TV ("100%", "100%%");
  TV ("a", "%c", 'a');
  TV ("ff", "%x", 255);
  TV ("17", "%o", 15);
  TV ("+5", "%+d", 5);
  TV (" 5", "% d", 5);
  TV ("   ab", "%5s", "ab");
  TV ("ab   ", "%-5s", "ab");
  TV ("00042", "%05d", 42);
  TV ("007", "%.3d", 7);
  TV ("hel", "%.*s", i, "hello");
  TV ("   ab", "%*s", 5, "ab");
  TV ("   ab", "%*s", w, "ab");
  TV ("ab   ", "%*s", -w, "ab");
  TV ("[   ab]", "[%*s]", w, "ab");
  TV ("   42", "%*d", w, 42);
  TV ("   he", "%*.2s", w, "hello");
  TV ("3", "%d", i);
  TV ("x3", "%s%d", "x", i);
  TV ("3x", "%d%s", i, "x");
}

/* Same as above but with a destination of unknown size, so that the calls
   are not rejected for possibly overflowing it.  */

static void __attribute__ ((noipa))
sprintf_width_s (char *d, int w, const char *s)
{
  sprintf (d, "%*s", w, s);
}

static void __attribute__ ((noipa))
sprintf_width_d (char *d, int w)
{
  sprintf (d, "%*d", w, 42);
}

static void __attribute__ ((noipa))
sprintf_width_prec_s (char *d, int w, const char *s)
{
  sprintf (d, "[%*.2s]", w, s);
}

/* Call FUNC (BUF, ARGS...) and check its output.  */
#define F(expect, func, ...)					\
  do {								\
    char buf[BUFSIZE];						\
    __builtin_memset (buf, 'X', sizeof buf);			\
    buf[sizeof buf - 1] = '\0';					\
    func (buf, __VA_ARGS__);					\
    check (__LINE__, buf, -1, expect);				\
  } while (0)

static void __attribute__ ((noipa))
test_unknown_width (int w)
{
  F ("   ab", sprintf_width_s, w, "ab");
  F ("ab   ", sprintf_width_s, -w, "ab");
  F ("ab", sprintf_width_s, 0, "ab");
  F ("   42", sprintf_width_d, w);
  F ("[   he]", sprintf_width_prec_s, w, "hello");
}

int
main (void)
{
  test_plain ();
  test_s_const ();
  test_s_const_prec ();
  test_s_range (vzero);
  test_s_range (vone);
  test_s_unknown ();
  test_unhandled (3, 5);
  test_unknown_width (5);

  /* Return failure rather than calling abort so that the output
     describing the failures is flushed.  */
  if (nfails)
    {
      __builtin_printf ("%u failures\n", nfails);
      return 1;
    }

  return 0;
}
