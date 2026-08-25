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

void exit (int);
int glob_int_arr[100];
int glob_int = 4;
volatile int glob_vol_int_arr[100];
int * volatile glob_vol_ptr_int = glob_int_arr;
volatile int *glob_ptr_vol_int = glob_vol_int_arr;
volatile int * volatile glob_vol_ptr_vol_int = glob_vol_int_arr;
volatile int glob_vol_int;
static int stat_int_arr[100];
static volatile int stat_vol_int_arr[100];
static int * volatile stat_vol_ptr_int = stat_int_arr;
static volatile int *stat_ptr_vol_int = stat_vol_int_arr;
static volatile int * volatile stat_vol_ptr_vol_int = stat_vol_int_arr;
static volatile int stat_vol_int;
struct S {
  int a;
  short b, c;
  char d[8];
  struct S *next;
};
struct S str;
volatile struct S vol_str;
struct S * volatile vol_ptr_str = &str;
volatile struct S *ptr_vol_str = &vol_str;
volatile struct S * volatile vol_ptr_vol_str = &vol_str;
void
simple_vol_global ()
{
  __builtin_prefetch (glob_vol_int_arr, 0, 0);
  __builtin_prefetch (glob_vol_ptr_int, 0, 0);
  __builtin_prefetch (glob_ptr_vol_int, 0, 0);
  __builtin_prefetch (glob_vol_ptr_vol_int, 0, 0);
  __builtin_prefetch (&glob_vol_int, 0, 0);
}
void
simple_vol_file ()
{
  __builtin_prefetch (stat_vol_int_arr, 0, 0);
  __builtin_prefetch (stat_vol_ptr_int, 0, 0);
  __builtin_prefetch (stat_ptr_vol_int, 0, 0);
  __builtin_prefetch (stat_vol_ptr_vol_int, 0, 0);
  __builtin_prefetch (&stat_vol_int, 0, 0);
}
void
expr_vol_global (void)
{
  __builtin_prefetch (&vol_str, 0, 0);
  __builtin_prefetch (ptr_vol_str, 0, 0);
  __builtin_prefetch (vol_ptr_str, 0, 0);
  __builtin_prefetch (vol_ptr_vol_str, 0, 0);
  __builtin_prefetch (&vol_str.b, 0, 0);
  __builtin_prefetch (&ptr_vol_str->b, 0, 0);
  __builtin_prefetch (&vol_ptr_str->b, 0, 0);
  __builtin_prefetch (&vol_ptr_vol_str->b, 0, 0);
  __builtin_prefetch (&vol_str.d, 0, 0);
  __builtin_prefetch (&vol_ptr_str->d, 0, 0);
  __builtin_prefetch (&ptr_vol_str->d, 0, 0);
  __builtin_prefetch (&vol_ptr_vol_str->d, 0, 0);
  __builtin_prefetch (vol_str.next, 0, 0);
  __builtin_prefetch (vol_ptr_str->next, 0, 0);
  __builtin_prefetch (ptr_vol_str->next, 0, 0);
  __builtin_prefetch (vol_ptr_vol_str->next, 0, 0);
  __builtin_prefetch (vol_str.next->d, 0, 0);
  __builtin_prefetch (vol_ptr_str->next->d, 0, 0);
  __builtin_prefetch (ptr_vol_str->next->d, 0, 0);
  __builtin_prefetch (vol_ptr_vol_str->next->d, 0, 0);
  __builtin_prefetch (&glob_vol_int_arr, 0, 0);
  __builtin_prefetch (glob_vol_ptr_int, 0, 0);
  __builtin_prefetch (glob_ptr_vol_int, 0, 0);
  __builtin_prefetch (glob_vol_ptr_vol_int, 0, 0);
  __builtin_prefetch (&glob_vol_int_arr[2], 0, 0);
  __builtin_prefetch (&glob_vol_ptr_int[3], 0, 0);
  __builtin_prefetch (&glob_ptr_vol_int[3], 0, 0);
  __builtin_prefetch (&glob_vol_ptr_vol_int[3], 0, 0);
  __builtin_prefetch (glob_vol_int_arr+3, 0, 0);
  __builtin_prefetch (glob_vol_int_arr+glob_vol_int, 0, 0);
  __builtin_prefetch (glob_vol_ptr_int+5, 0, 0);
  __builtin_prefetch (glob_ptr_vol_int+5, 0, 0);
  __builtin_prefetch (glob_vol_ptr_vol_int+5, 0, 0);
  __builtin_prefetch (glob_vol_ptr_int+glob_vol_int, 0, 0);
  __builtin_prefetch (glob_ptr_vol_int+glob_vol_int, 0, 0);
  __builtin_prefetch (glob_vol_ptr_vol_int+glob_vol_int, 0, 0);
}
int
main ()
{
  simple_vol_global ();
  simple_vol_file ();
  str.next = &str;
  vol_str.next = &str;
  expr_vol_global ();
  exit (0);
}
