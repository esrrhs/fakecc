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
int check_fa_work (const char *, const char *) __attribute__((noinline,noipa));
int check_fa_mid (const char *) __attribute__((noinline,noipa));
int check_fa (char *) __attribute__((noinline,noipa));
int how_much (void) __attribute__((noinline,noipa));
int check_fa_work (const char *c, const char *f)
{
  const char d = 0;
  if (c >= &d)
    return c >= f && f >= &d;
  else
    return c <= f && f <= &d;
}
int check_fa_mid (const char *c)
{
  const char *f = __builtin_frame_address (0);
  return check_fa_work (c, f) != 0;
}
int check_fa (char *unused)
{
  const char c = 0;
  return check_fa_mid (&c) != 0;
}
int how_much (void)
{
 return 8;
}
int main (void)
{
  char *unused = __builtin_alloca (how_much ());
  if (!check_fa(unused))
    abort();
  return 0;
}
