// expect: 0
package main;

typedef union
{
  int __lock;
} pthread_mutex_t;

extern void abort(void);

int main(void)
{
    struct { int c; pthread_mutex_t m; } r = { .m = 0 };
    if (r.c != 0)
      abort ();
    return 0;
}
