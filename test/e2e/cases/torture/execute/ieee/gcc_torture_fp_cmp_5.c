// expect: 0
package main;

/* Like fp-cmp-4.c, but test that the setcc patterns are correct.  */

void abort (void);
void exit (int);

static int
test_isunordered(double x, double y)
{
  return __builtin_isunordered(x, y);
}

static int
test_not_isunordered(double x, double y)
{
  return !__builtin_isunordered(x, y);
}

static int
test_isless(double x, double y)
{
  return __builtin_isless(x, y);
}

static int
test_not_isless(double x, double y)
{
  return !__builtin_isless(x, y);
}

static int
test_islessequal(double x, double y)
{
  return __builtin_islessequal(x, y);
}

static int
test_not_islessequal(double x, double y)
{
  return !__builtin_islessequal(x, y);
}

static int
test_isgreater(double x, double y)
{
  return __builtin_isgreater(x, y);
}

static int
test_not_isgreater(double x, double y)
{
  return !__builtin_isgreater(x, y);
}

static int
test_isgreaterequal(double x, double y)
{
  return __builtin_isgreaterequal(x, y);
}

static int
test_not_isgreaterequal(double x, double y)
{
  return !__builtin_isgreaterequal(x, y);
}

static int
test_islessgreater(double x, double y)
{
  return __builtin_islessgreater(x, y);
}

static int
test_not_islessgreater(double x, double y)
{
  return !__builtin_islessgreater(x, y);
}

static void
one_test(double x, double y, int expected,
         int (*pos) (double, double), int (*neg) (double, double))
{
  if ((*pos)(x, y) != expected)
    abort ();
  if ((*neg)(x, y) != !expected)
    abort ();
}

int
main()
{
  double NAN = 0.0 / 0.0;

  struct try
  {
    double x, y;
    int result[6];
  };

  struct try data[6];
  data[0].x = NAN; data[0].y = NAN; data[0].result[0] = 1; data[0].result[1] = 0; data[0].result[2] = 0; data[0].result[3] = 0; data[0].result[4] = 0; data[0].result[5] = 0;
  data[1].x = 0.0; data[1].y = NAN; data[1].result[0] = 1; data[1].result[1] = 0; data[1].result[2] = 0; data[1].result[3] = 0; data[1].result[4] = 0; data[1].result[5] = 0;
  data[2].x = NAN; data[2].y = 0.0; data[2].result[0] = 1; data[2].result[1] = 0; data[2].result[2] = 0; data[2].result[3] = 0; data[2].result[4] = 0; data[2].result[5] = 0;
  data[3].x = 0.0; data[3].y = 0.0; data[3].result[0] = 0; data[3].result[1] = 0; data[3].result[2] = 1; data[3].result[3] = 0; data[3].result[4] = 1; data[3].result[5] = 0;
  data[4].x = 1.0; data[4].y = 2.0; data[4].result[0] = 0; data[4].result[1] = 1; data[4].result[2] = 1; data[4].result[3] = 0; data[4].result[4] = 0; data[4].result[5] = 1;
  data[5].x = 2.0; data[5].y = 1.0; data[5].result[0] = 0; data[5].result[1] = 0; data[5].result[2] = 0; data[5].result[3] = 1; data[5].result[4] = 1; data[5].result[5] = 1;

  struct test
  {
    int (*pos)(double, double);
    int (*neg)(double, double);
  };

  struct test tests[6];
  tests[0].pos = test_isunordered; tests[0].neg = test_not_isunordered;
  tests[1].pos = test_isless; tests[1].neg = test_not_isless;
  tests[2].pos = test_islessequal; tests[2].neg = test_not_islessequal;
  tests[3].pos = test_isgreater; tests[3].neg = test_not_isgreater;
  tests[4].pos = test_isgreaterequal; tests[4].neg = test_not_isgreaterequal;
  tests[5].pos = test_islessgreater; tests[5].neg = test_not_islessgreater;

  const int n = 6;
  int i, j;

  for (i = 0; i < n; ++i)
    for (j = 0; j < 6; ++j)
      one_test (data[i].x, data[i].y, data[i].result[j],
		tests[j].pos, tests[j].neg);

  exit (0);
}
