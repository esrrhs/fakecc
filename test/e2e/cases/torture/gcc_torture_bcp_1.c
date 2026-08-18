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

extern void abort (void);
extern void exit (int);
__attribute__ ((externally_visible)) int global;
int func(void);
int bad0(void) { return __builtin_constant_p(global); }
int bad1(void) { return __builtin_constant_p(global++); }
inline int bad2(int x) { return __builtin_constant_p(x++); }
inline int bad3(int x) { return __builtin_constant_p(x); }
inline int bad4(const char *x) { return __builtin_constant_p(x); }
int bad5(void) { return bad2(1); }
inline int bad6(int x) { return __builtin_constant_p(x+1); }
int bad7(void) { return __builtin_constant_p(func()); }
int bad8(void) { char buf[10]; return __builtin_constant_p(buf); }
int bad9(const char *x) { return __builtin_constant_p(x[123456]); }
int bad10(void) { return __builtin_constant_p(&global); }
int good0(void) { return __builtin_constant_p(1); }
int good1(void) { return __builtin_constant_p("hi"); }
int good2(void) { return __builtin_constant_p((1234 + 45) & ~7); }
int opt0(void) { return bad3(1); }
int opt1(void) { return bad6(1); }
int opt2(void) { return __builtin_constant_p("hi"[0]); }
int (* volatile bad_t0[])(void) = {
 bad0, bad1, bad5, bad7, bad8, bad10
};
int (* volatile bad_t1[])(int x) = {
 bad2, bad3, bad6
};
int (* volatile bad_t2[])(const char *x) = {
 bad4, bad9
};
int (* volatile good_t0[])(void) = {
 good0, good1, good2
};
int (* volatile opt_t0[])(void) = {
 opt0, opt1, opt2
};
int main()
{
  int i;
  for (i = 0; i < (sizeof(bad_t0)/sizeof(*bad_t0)); ++i)
    if ((*bad_t0[i])())
      abort();
  for (i = 0; i < (sizeof(bad_t1)/sizeof(*bad_t1)); ++i)
    if ((*bad_t1[i])(1))
      abort();
  for (i = 0; i < (sizeof(bad_t2)/sizeof(*bad_t2)); ++i)
    if ((*bad_t2[i])("hi"))
      abort();
  for (i = 0; i < (sizeof(good_t0)/sizeof(*good_t0)); ++i)
    if (! (*good_t0[i])())
      abort();
  exit(0);
}
