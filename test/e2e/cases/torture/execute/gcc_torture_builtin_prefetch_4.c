// expect: 0
package main;

extern void* memcpy(void*, const void*, unsigned long);
extern void* memset(void*, int, unsigned long);
extern int memcmp(const void*, const void*, unsigned long);
extern void* memmove(void*, const void*, unsigned long);
extern int strcmp(const char*, const char*);
extern int strncmp(const char*, const char*, unsigned long);
extern unsigned long strlen(const char*);
extern char* strcpy(char*, const char*);
extern char* strncpy(char*, const char*, unsigned long);
extern char* strchr(const char*, int);
extern char* strrchr(const char*, int);
extern char* strcat(char*, const char*);
extern char* strncat(char*, const char*, unsigned long);
extern char* strstr(const char*, const char*);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int puts(const char*);
extern int putchar(int);
extern void* malloc(unsigned long);
extern void free(void*);
extern void* calloc(unsigned long, unsigned long);
extern void* realloc(void*, unsigned long);
extern int abs(int);
extern long labs(long);
extern int atoi(const char*);
extern long atol(const char*);
extern double atof(const char*);
extern double sqrt(double);
extern double fabs(double);
extern double pow(double, double);
extern double ceil(double);
extern double floor(double);
extern void exit(int);
extern void abort(void);
extern int rand(void);
extern void srand(unsigned int);

void abort (void);
void exit (int);
int arr[100];
int *ptr = &arr[20];
int arrindex = 4;
int
assign_arg_ptr (int *p)
{
  int *q;
  __builtin_prefetch ((q = p), 0, 0);
  return q == p;
}
int
assign_glob_ptr (void)
{
  int *q;
  __builtin_prefetch ((q = ptr), 0, 0);
  return q == ptr;
}
int
assign_arg_idx (int *p, int i)
{
  int j;
  __builtin_prefetch (&p[j = i], 0, 0);
  return j == i;
}
int
assign_glob_idx (void)
{
  int j;
  __builtin_prefetch (&ptr[j = arrindex], 0, 0);
  return j == arrindex;
}
int
preinc_arg_ptr (int *p)
{
  int *q;
  q = p + 1;
  __builtin_prefetch (++p, 0, 0);
  return p == q;
}
int
preinc_glob_ptr (void)
{
  int *q;
  q = ptr + 1;
  __builtin_prefetch (++ptr, 0, 0);
  return ptr == q;
}
int
postinc_arg_ptr (int *p)
{
  int *q;
  q = p + 1;
  __builtin_prefetch (p++, 0, 0);
  return p == q;
}
int
postinc_glob_ptr (void)
{
  int *q;
  q = ptr + 1;
  __builtin_prefetch (ptr++, 0, 0);
  return ptr == q;
}
int
predec_arg_ptr (int *p)
{
  int *q;
  q = p - 1;
  __builtin_prefetch (--p, 0, 0);
  return p == q;
}
int
predec_glob_ptr (void)
{
  int *q;
  q = ptr - 1;
  __builtin_prefetch (--ptr, 0, 0);
  return ptr == q;
}
int
postdec_arg_ptr (int *p)
{
  int *q;
  q = p - 1;
  __builtin_prefetch (p--, 0, 0);
  return p == q;
}
int
postdec_glob_ptr (void)
{
  int *q;
  q = ptr - 1;
  __builtin_prefetch (ptr--, 0, 0);
  return ptr == q;
}
int
preinc_arg_idx (int *p, int i)
{
  int j = i + 1;
  __builtin_prefetch (&p[++i], 0, 0);
  return i == j;
}
int
preinc_glob_idx (void)
{
  int j = arrindex + 1;
  __builtin_prefetch (&ptr[++arrindex], 0, 0);
  return arrindex == j;
}
int
postinc_arg_idx (int *p, int i)
{
  int j = i + 1;
  __builtin_prefetch (&p[i++], 0, 0);
  return i == j;
}
int
postinc_glob_idx (void)
{
  int j = arrindex + 1;
  __builtin_prefetch (&ptr[arrindex++], 0, 0);
  return arrindex == j;
}
int
predec_arg_idx (int *p, int i)
{
  int j = i - 1;
  __builtin_prefetch (&p[--i], 0, 0);
  return i == j;
}
int
predec_glob_idx (void)
{
  int j = arrindex - 1;
  __builtin_prefetch (&ptr[--arrindex], 0, 0);
  return arrindex == j;
}
int
postdec_arg_idx (int *p, int i)
{
  int j = i - 1;
  __builtin_prefetch (&p[i--], 0, 0);
  return i == j;
}
int
postdec_glob_idx (void)
{
  int j = arrindex - 1;
  __builtin_prefetch (&ptr[arrindex--], 0, 0);
  return arrindex == j;
}
int getptrcnt = 0;
int *
getptr (int *p)
{
  getptrcnt++;
  return p + 1;
}
int
funccall_arg_ptr (int *p)
{
  __builtin_prefetch (getptr (p), 0, 0);
  return getptrcnt == 1;
}
int getintcnt = 0;
int
getint (int i)
{
  getintcnt++;
  return i + 1;
}
int
funccall_arg_idx (int *p, int i)
{
  __builtin_prefetch (&p[getint (i)], 0, 0);
  return getintcnt == 1;
}
int
main ()
{
  if (!assign_arg_ptr (ptr))
    abort ();
  if (!assign_glob_ptr ())
    abort ();
  if (!assign_arg_idx (ptr, 4))
    abort ();
  if (!assign_glob_idx ())
    abort ();
  if (!preinc_arg_ptr (ptr))
    abort ();
  if (!preinc_glob_ptr ())
    abort ();
  if (!postinc_arg_ptr (ptr))
    abort ();
  if (!postinc_glob_ptr ())
    abort ();
  if (!predec_arg_ptr (ptr))
    abort ();
  if (!predec_glob_ptr ())
    abort ();
  if (!postdec_arg_ptr (ptr))
    abort ();
  if (!postdec_glob_ptr ())
    abort ();
  if (!preinc_arg_idx (ptr, 3))
    abort ();
  if (!preinc_glob_idx ())
    abort ();
  if (!postinc_arg_idx (ptr, 3))
    abort ();
  if (!postinc_glob_idx ())
    abort ();
  if (!predec_arg_idx (ptr, 3))
    abort ();
  if (!predec_glob_idx ())
    abort ();
  if (!postdec_arg_idx (ptr, 3))
    abort ();
  if (!postdec_glob_idx ())
    abort ();
  if (!funccall_arg_ptr (ptr))
    abort ();
  if (!funccall_arg_idx (ptr, 3))
    abort ();
  exit (0);
}
