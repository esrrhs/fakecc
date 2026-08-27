// expect: 0
package main;

/* The purpose of this code is to test argument passing of a tuple of
   11 integers, with the break point between named and unnamed arguments
   at every possible position.	*/

extern void abort(void);
extern void exit(int);
extern int printf(const char *, ...);

static int errors = 0;
static void
verify (const char *tcase, int n[11])
{
  int i;
  for (i = 0; i <= 10; i++)
    if (n[i] != i)
      {
        printf (" %s: n[%d] = %d expected %d\n", tcase, i, n[i], i);
        errors++;
      }
}

static void varargs0 (int q0, ...) { va_list ap; int n[11]; int i; va_start(ap, q0); n[0] = q0; for (i = 0 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs0", n); }
static void varargs1 (int q0, int q1, ...) { va_list ap; int n[11]; int i; va_start(ap, q1); n[0] = q0; n[1] = q1; for (i = 1 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs1", n); }
static void varargs2 (int q0, int q1, int q2, ...) { va_list ap; int n[11]; int i; va_start(ap, q2); n[0] = q0; n[1] = q1; n[2] = q2; for (i = 2 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs2", n); }
static void varargs3 (int q0, int q1, int q2, int q3, ...) { va_list ap; int n[11]; int i; va_start(ap, q3); n[0] = q0; n[1] = q1; n[2] = q2; n[3] = q3; for (i = 3 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs3", n); }
static void varargs4 (int q0, int q1, int q2, int q3, int q4, ...) { va_list ap; int n[11]; int i; va_start(ap, q4); n[0] = q0; n[1] = q1; n[2] = q2; n[3] = q3; n[4] = q4; for (i = 4 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs4", n); }
static void varargs5 (int q0, int q1, int q2, int q3, int q4, int q5, ...) { va_list ap; int n[11]; int i; va_start(ap, q5); n[0] = q0; n[1] = q1; n[2] = q2; n[3] = q3; n[4] = q4; n[5] = q5; for (i = 5 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs5", n); }
static void varargs6 (int q0, int q1, int q2, int q3, int q4, int q5, int q6, ...) { va_list ap; int n[11]; int i; va_start(ap, q6); n[0] = q0; n[1] = q1; n[2] = q2; n[3] = q3; n[4] = q4; n[5] = q5; n[6] = q6; for (i = 6 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs6", n); }
static void varargs7 (int q0, int q1, int q2, int q3, int q4, int q5, int q6, int q7, ...) { va_list ap; int n[11]; int i; va_start(ap, q7); n[0] = q0; n[1] = q1; n[2] = q2; n[3] = q3; n[4] = q4; n[5] = q5; n[6] = q6; n[7] = q7; for (i = 7 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs7", n); }
static void varargs8 (int q0, int q1, int q2, int q3, int q4, int q5, int q6, int q7, int q8, ...) { va_list ap; int n[11]; int i; va_start(ap, q8); n[0] = q0; n[1] = q1; n[2] = q2; n[3] = q3; n[4] = q4; n[5] = q5; n[6] = q6; n[7] = q7; n[8] = q8; for (i = 8 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs8", n); }
static void varargs9 (int q0, int q1, int q2, int q3, int q4, int q5, int q6, int q7, int q8, int q9, ...) { va_list ap; int n[11]; int i; va_start(ap, q9); n[0] = q0; n[1] = q1; n[2] = q2; n[3] = q3; n[4] = q4; n[5] = q5; n[6] = q6; n[7] = q7; n[8] = q8; n[9] = q9; for (i = 9 + 1; i <= 10; i++) n[i] = va_arg(ap,int); va_end(ap); verify ("varargs9", n); }

int main(void)
{
  varargs0 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs1 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs2 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs3 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs4 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs5 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs6 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs7 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs8 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  varargs9 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  if (errors)
    abort ();
  exit (0);
}
