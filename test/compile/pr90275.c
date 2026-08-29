/* PR c/90275 — implicit-int / comma-expr at file scope requires K&R mode. */
int a, b, c;

long long d;

int e() {

  char f;

  for (;;) {

    c = a = c ? 5 : 0;

    if (f) {

      b = a;

      f = d;

    }

    (d || b) < (a > e) ?: (b ? 0 : f) || (d -= f);

  }

  return 0;
}
