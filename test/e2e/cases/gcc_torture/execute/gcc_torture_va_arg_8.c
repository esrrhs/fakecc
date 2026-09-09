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

typedef long long int INT64;

static inline void
debug(int i1, int i2, int i3, int i4, int i5,
      int i6, int i7, int i8, int i9, ...)
{
  va_list ap;

  va_start (ap, i9);

  if (va_arg (ap,int) != 10)
    abort ();
  if (va_arg (ap,INT64) != 0x123400005678LL)
    abort ();

  va_end (ap);
}

int
main(void)
{
  debug(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0x123400005678LL);
  exit(0);
}
