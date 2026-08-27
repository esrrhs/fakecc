// expect: 0
package main;

void abort (void);
void exit (int);

typedef double FLOAT;

/* Like fp-cmp-4.c, but test that the cmove patterns are correct.  */

static FLOAT
test_isunordered(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return __builtin_isunordered(x, y) ? a : b;
}

static FLOAT
test_not_isunordered(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return !__builtin_isunordered(x, y) ? a : b;
}

static FLOAT
test_isless(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return __builtin_isless(x, y) ? a : b;
}

static FLOAT
test_not_isless(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return !__builtin_isless(x, y) ? a : b;
}

static FLOAT
test_islessequal(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return __builtin_islessequal(x, y) ? a : b;
}

static FLOAT
test_not_islessequal(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return !__builtin_islessequal(x, y) ? a : b;
}

static FLOAT
test_isgreater(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return __builtin_isgreater(x, y) ? a : b;
}

static FLOAT
test_not_isgreater(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return !__builtin_isgreater(x, y) ? a : b;
}

static FLOAT
test_isgreaterequal(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return __builtin_isgreaterequal(x, y) ? a : b;
}

static FLOAT
test_not_isgreaterequal(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return !__builtin_isgreaterequal(x, y) ? a : b;
}

static FLOAT
test_islessgreater(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return __builtin_islessgreater(x, y) ? a : b;
}

static FLOAT
test_not_islessgreater(FLOAT x, FLOAT y, FLOAT a, FLOAT b)
{
  return !__builtin_islessgreater(x, y) ? a : b;
}

static void
one_test(FLOAT x, FLOAT y, int expected,
         FLOAT (*pos) (FLOAT, FLOAT, FLOAT, FLOAT), 
	 FLOAT (*neg) (FLOAT, FLOAT, FLOAT, FLOAT))
{
  if (((*pos)(x, y, 1.0, 2.0) == 1.0) != expected)
    abort ();
  if (((*neg)(x, y, 3.0, 4.0) == 4.0) != expected)
    abort ();
}

int
main()
{
  FLOAT NAN = 0.0 / 0.0;
  FLOAT INF = 1.0 / 0.0;

  struct try
  {
    FLOAT x, y;
    int result[6];
  };

  struct try data[14];
  data[0].x = NAN;  data[0].y = NAN;  data[0].result[0] = 1; data[0].result[1] = 0; data[0].result[2] = 0; data[0].result[3] = 0; data[0].result[4] = 0; data[0].result[5] = 0;
  data[1].x = 0.0;  data[1].y = NAN;  data[1].result[0] = 1; data[1].result[1] = 0; data[1].result[2] = 0; data[1].result[3] = 0; data[1].result[4] = 0; data[1].result[5] = 0;
  data[2].x = NAN;  data[2].y = 0.0;  data[2].result[0] = 1; data[2].result[1] = 0; data[2].result[2] = 0; data[2].result[3] = 0; data[2].result[4] = 0; data[2].result[5] = 0;
  data[3].x = 0.0;  data[3].y = 0.0;  data[3].result[0] = 0; data[3].result[1] = 0; data[3].result[2] = 1; data[3].result[3] = 0; data[3].result[4] = 1; data[3].result[5] = 0;
  data[4].x = 1.0;  data[4].y = 2.0;  data[4].result[0] = 0; data[4].result[1] = 1; data[4].result[2] = 1; data[4].result[3] = 0; data[4].result[4] = 0; data[4].result[5] = 1;
  data[5].x = 2.0;  data[5].y = 1.0;  data[5].result[0] = 0; data[5].result[1] = 0; data[5].result[2] = 0; data[5].result[3] = 1; data[5].result[4] = 1; data[5].result[5] = 1;
  data[6].x = INF;  data[6].y = 0.0;  data[6].result[0] = 0; data[6].result[1] = 0; data[6].result[2] = 0; data[6].result[3] = 1; data[6].result[4] = 1; data[6].result[5] = 1;
  data[7].x = 1.0;  data[7].y = INF;  data[7].result[0] = 0; data[7].result[1] = 1; data[7].result[2] = 1; data[7].result[3] = 0; data[7].result[4] = 0; data[7].result[5] = 1;
  data[8].x = INF;  data[8].y = INF;  data[8].result[0] = 0; data[8].result[1] = 0; data[8].result[2] = 1; data[8].result[3] = 0; data[8].result[4] = 1; data[8].result[5] = 0;
  data[9].x = 0.0;  data[9].y = -INF; data[9].result[0] = 0; data[9].result[1] = 0; data[9].result[2] = 0; data[9].result[3] = 1; data[9].result[4] = 1; data[9].result[5] = 1;
  data[10].x = -INF; data[10].y = 1.0; data[10].result[0] = 0; data[10].result[1] = 1; data[10].result[2] = 1; data[10].result[3] = 0; data[10].result[4] = 0; data[10].result[5] = 1;
  data[11].x = -INF; data[11].y = -INF;data[11].result[0] = 0; data[11].result[1] = 0; data[11].result[2] = 1; data[11].result[3] = 0; data[11].result[4] = 1; data[11].result[5] = 0;
  data[12].x = INF;  data[12].y = -INF;data[12].result[0] = 0; data[12].result[1] = 0; data[12].result[2] = 0; data[12].result[3] = 1; data[12].result[4] = 1; data[12].result[5] = 1;
  data[13].x = -INF; data[13].y = INF; data[13].result[0] = 0; data[13].result[1] = 1; data[13].result[2] = 1; data[13].result[3] = 0; data[13].result[4] = 0; data[13].result[5] = 1;

  struct test
  {
    FLOAT (*pos)(FLOAT, FLOAT, FLOAT, FLOAT);
    FLOAT (*neg)(FLOAT, FLOAT, FLOAT, FLOAT);
  };

  struct test tests[6];
  tests[0].pos = test_isunordered; tests[0].neg = test_not_isunordered;
  tests[1].pos = test_isless; tests[1].neg = test_not_isless;
  tests[2].pos = test_islessequal; tests[2].neg = test_not_islessequal;
  tests[3].pos = test_isgreater; tests[3].neg = test_not_isgreater;
  tests[4].pos = test_isgreaterequal; tests[4].neg = test_not_isgreaterequal;
  tests[5].pos = test_islessgreater; tests[5].neg = test_not_islessgreater;

  const int n = 14;
  int i, j;

  for (i = 0; i < n; ++i)
    for (j = 0; j < 6; ++j)
      one_test (data[i].x, data[i].y, data[i].result[j],
		tests[j].pos, tests[j].neg);

  exit (0);
}
