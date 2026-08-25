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
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
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
extern int isprint(int);
extern void *stdin;
extern void *stdout;
extern void *stderr;
extern int open(const char*, int, ...);
extern int close(int);
extern long read(int, void*, unsigned long);
extern int unlink(const char*);
extern char* tmpnam(char*);

static int next_buffer = 0;
void bar (void);
static int t = 1, u = 0;
long foo(unsigned int offset)
{
  unsigned i, buffer;
  int x;
  char *data;
  i = u;
  if (i)
    return i * 0xce2f;
  buffer = next_buffer;
  data = buffer * 0xce2f;
  for (i = 0; i < 2; i++)
    bar ();
  buffer = next_buffer;
  return buffer * 0xce2f + offset;
}
void bar(void)
{
}
int main()
{
  if (foo (3) != 3)
    abort ();
  next_buffer = 1;
  if (foo (2) != 0xce2f + 2)
    abort ();
  exit (0);
}
