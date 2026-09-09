// expect: 0
package main;

/* Test to verify that strlen() calls with conditional expressions
   and unterminated arrays or pointers to such things as arguments
   are evaluated without making assumptions about array sizes.  */

extern unsigned long strlen (const char *);
extern int printf (const char *, ...);
extern void abort (void);

unsigned nfails;

void check (const char *expr, const char *s, unsigned actual, unsigned expected, int line)
{
  if (actual != expected)
    {
      printf ("line %d: strlen (%s = \"%s\") == %u (got %u) failed\n", line, expr, s, expected, actual);
      nfails++;
    }
}

volatile int i0 = 0;

const char ca[2][3] = { "12" };
const char cb[2][3] = { { '1', '2', '3', }, { '4' } };

char va[2][3] = { "123" };
char vb[2][3] = { { '1', '2', '3', }, { '4', '5' } };

const char *s = "123456";

static void test_binary_cond_expr_global (void)
{
  check ("i0 ? \"1\" : ca[0]", i0 ? "1" : ca[0], strlen (i0 ? "1" : ca[0]), 2, 35);
  check ("i0 ? ca[0] : \"123\"", i0 ? ca[0] : "123", strlen (i0 ? ca[0] : "123"), 3, 36);

  /* The call to strlen (cb[0]) is strictly undefined because the array
     isn't nul-terminated.  This test verifies that the strlen range
     optimization doesn't assume that the argument is necessarily nul
     terminated.
     Ditto for strlen (vb[0]).  */
  check ("i0 ? \"1\" : cb[0]", i0 ? "1" : cb[0], strlen (i0 ? "1" : cb[0]), 4, 43);
  check ("i0 ? cb[0] : \"12\"", i0 ? cb[0] : "12", strlen (i0 ? cb[0] : "12"), 2, 44);

  check ("i0 ? \"1\" : va[0]", i0 ? "1" : va[0], strlen (i0 ? "1" : va[0]), 3, 46);
  check ("i0 ? va[0] : \"1234\"", i0 ? va[0] : "1234", strlen (i0 ? va[0] : "1234"), 4, 47);

  check ("i0 ? \"1\" : vb[0]", i0 ? "1" : vb[0], strlen (i0 ? "1" : vb[0]), 5, 49);
  check ("i0 ? vb[0] : \"12\"", i0 ? vb[0] : "12", strlen (i0 ? vb[0] : "12"), 2, 50);
}

static void test_binary_cond_expr_local (void)
{
  const char lca[2][3] = { "12" };
  const char lcb[2][3] = { { '1', '2', '3', }, { '4' } };

  char lva[2][3] = { "123" };
  char lvb[2][3] = { { '1', '2', '3', }, { '4', '5' } };

  check ("i0 ? \"1\" : lca[0]", i0 ? "1" : lca[0], strlen (i0 ? "1" : lca[0]), 2, 63);
  check ("i0 ? lca[0] : \"123\"", i0 ? lca[0] : "123", strlen (i0 ? lca[0] : "123"), 3, 64);

  check ("i0 ? \"1\" : lcb[0]", i0 ? "1" : lcb[0], strlen (i0 ? "1" : lcb[0]), 4, 66);
  check ("i0 ? lcb[0] : \"12\"", i0 ? lcb[0] : "12", strlen (i0 ? lcb[0] : "12"), 2, 67);

  check ("i0 ? \"1\" : lva[0]", i0 ? "1" : lva[0], strlen (i0 ? "1" : lva[0]), 3, 69);
  check ("i0 ? lva[0] : \"1234\"", i0 ? lva[0] : "1234", strlen (i0 ? lva[0] : "1234"), 4, 70);

  check ("i0 ? \"1\" : lvb[0]", i0 ? "1" : lvb[0], strlen (i0 ? "1" : lvb[0]), 5, 72);
  check ("i0 ? lvb[0] : \"12\"", i0 ? lvb[0] : "12", strlen (i0 ? lvb[0] : "12"), 2, 73);
}

static void test_ternary_cond_expr (void)
{
  check ("i0 == 0 ? s : i0 == 1 ? vb[0] : \"123\"", i0 == 0 ? s : i0 == 1 ? vb[0] : "123", strlen (i0 == 0 ? s : i0 == 1 ? vb[0] : "123"), 6, 80);
  check ("i0 == 0 ? vb[0] : i0 == 1 ? s : \"123\"", i0 == 0 ? vb[0] : i0 == 1 ? s : "123", strlen (i0 == 0 ? vb[0] : i0 == 1 ? s : "123"), 5, 81);
  check ("i0 == 0 ? \"123\" : i0 == 1 ? s : vb[0]", i0 == 0 ? "123" : i0 == 1 ? s : vb[0], strlen (i0 == 0 ? "123" : i0 == 1 ? s : vb[0]), 3, 82);
}

const char (*pca)[3] = &ca[0];
const char (*pcb)[3] = &cb[0];

char (*pva)[3] = &va[0];
char (*pvb)[3] = &vb[0];

static void test_binary_cond_expr_arrayptr (void)
{
  check ("i0 ? *pca : *pcb", i0 ? *pca : *pcb, strlen (i0 ? *pca : *pcb), 4, 95);
  check ("i0 ? *pcb : *pca", i0 ? *pcb : *pca, strlen (i0 ? *pcb : *pca), 2, 96);

  check ("i0 ? *pva : *pvb", i0 ? *pva : *pvb, strlen (i0 ? *pva : *pvb), 5, 98);
  check ("i0 ? *pvb : *pva", i0 ? *pvb : *pva, strlen (i0 ? *pvb : *pva), 3, 99);
}

int main (void)
{
  test_binary_cond_expr_global ();
  test_binary_cond_expr_local ();
  test_ternary_cond_expr ();
  test_binary_cond_expr_arrayptr ();

  if (nfails)
    abort ();
  return 0;
}
