// expect: 0
package main;

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
extern void abort (void);
void *
memcpy (void *dst, const void *src, long unsigned int n)
{
  const char *srcp;
  char *dstp;
  srcp = src;
  dstp = dst;
  if (dst < src)
    {
      if (dst + n > src)
 abort ();
    }
  else
    {
      if (src + n > dst)
 abort ();
    }
  while (n-- != 0)
    *dstp++ = *srcp++;
  return dst;
}
extern void abort (void);
extern void exit (int);
struct s { unsigned char a[256]; };
union u { struct { struct s b; int c; } d; struct { int c; struct s b; } e; };
static union u v;
static union u v0;
static struct s *p = &v.d.b;
static struct s *q = &v.e.b;
static inline struct s rp (void) { return *p; }
static inline struct s rq (void) { return *q; }
static void pq (void) { *p = rq(); }
static void qp (void) { *q = rp(); }
static void
init (struct s *sp)
{
  int i;
  for (i = 0; i < 256; i++)
    sp->a[i] = i;
}
static void
check (struct s *sp)
{
  int i;
  for (i = 0; i < 256; i++)
    if (sp->a[i] != i)
      abort ();
}
void
main_test (void)
{
  v = v0;
  init (p);
  qp ();
  check (q);
  v = v0;
  init (q);
  pq ();
  check (p);
  exit (0);
}

int main (void)
{
  inside_main = 1;
  main_test ();
  inside_main = 0;
  return 0;
}

void link_error (void)
{
  abort ();
}
