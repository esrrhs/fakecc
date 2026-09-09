// expect: 0
package main;

extern void abort(void);
extern void exit(int);
void abort(void);

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_list_tag;
typedef __va_list_tag __builtin_va_list[1];
typedef __builtin_va_list va_list;

static inline void
debug(int i1, int i2, int i3, int i4, int i5, int i6, int i7,
      double f1, double f2, double f3, double f4, double f5,
      double f6, double f7, double f8, double f9, ...)
{
  va_list ap;

  va_start (ap, f9);

  if (va_arg (ap,int) != 8)
    abort ();
  if (va_arg (ap,int) != 9)
    abort ();
  if (va_arg (ap,int) != 10)
    abort ();

  va_end (ap);
}

int
main (void)
{
  debug (1, 2, 3, 4, 5, 6, 7,
	 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
	 8, 9, 10);
  exit (0);
}
