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

void abort(void);

/* Baselines.  */
struct {
  char a1c;
  char *a1p;
} a1 = {
  '4',
  "62"
};

struct {
  char a2c;
  char a2p[2];
} a2 = {
  'v',
  "cq"
};

/* The tests.  */
struct {
  char a3c;
  char a3p[];
} a3 = {
  'o',
  "wx"
};

struct {
  char a4c;
  char a4p[];
} a4 = {
  '9',
  { 'e', 'b' }
};

int
main(void)
{
  if (a1.a1c != '4')
    abort();
  if (a1.a1p[0] != '6')
    abort();
  if (a1.a1p[1] != '2')
    abort();
  if (a1.a1p[2] != '\0')
    abort();

  if (a2.a2c != 'v')
    abort();
  if (a2.a2p[0] != 'c')
    abort();
  if (a2.a2p[1] != 'q')
    abort();

  if (a3.a3c != 'o')
    abort();
  if (a3.a3p[0] != 'w')
    abort();
  if (a3.a3p[1] != 'x')
    abort();

  if (a4.a4c != '9')
    abort();
  if (a4.a4p[0] != 'e')
    abort();
  if (a4.a4p[1] != 'b')
    abort();

  return 0;
}
