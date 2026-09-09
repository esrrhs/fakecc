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

void abort (void);
void exit (int);
float fp_add (float a, float b) { return a + b; }
float fp_sub (float a, float b) { return a - b; }
float fp_mul (float a, float b) { return a * b; }
float fp_div (float a, float b) { return a / b; }
float fp_neg (float a) { return -a; }
double dp_add (double a, double b) { return a + b; }
double dp_sub (double a, double b) { return a - b; }
double dp_mul (double a, double b) { return a * b; }
double dp_div (double a, double b) { return a / b; }
double dp_neg (double a) { return -a; }
double fp_to_dp (float f) { return f; }
float dp_to_fp (double d) { return d; }
int eqsf2 (float a, float b) { return a == b; }
int nesf2 (float a, float b) { return a != b; }
int gtsf2 (float a, float b) { return a > b; }
int gesf2 (float a, float b) { return a >= b; }
int ltsf2 (float a, float b) { return a < b; }
int lesf2 (float a, float b) { return a <= b; }
int eqdf2 (double a, double b) { return a == b; }
int nedf2 (double a, double b) { return a != b; }
int gtdf2 (double a, double b) { return a > b; }
int gedf2 (double a, double b) { return a >= b; }
int ltdf2 (double a, double b) { return a < b; }
int ledf2 (double a, double b) { return a <= b; }
float floatsisf (int i) { return i; }
double floatsidf (int i) { return i; }
int fixsfsi (float f) { return f; }
int fixdfsi (double d) { return d; }
unsigned int fixunssfsi (float f) { return f; }
unsigned int fixunsdfsi (double d) { return d; }
int fail_count = 0;
int fail(char *msg)
{
  fail_count++;
  fprintf (stderr, "Test failed: %s\n", msg);
}
int main()
{
  if (fp_add (1, 1) != 2) fail ("fp_add 1+1");
  if (fp_sub (3, 2) != 1) fail ("fp_sub 3-2");
  if (fp_mul (2, 3) != 6) fail ("fp_mul 2*3");
  if (fp_div (3, 2) != 1.5) fail ("fp_div 3/2");
  if (fp_neg (1) != -1) fail ("fp_neg 1");
  if (dp_add (1, 1) != 2) fail ("dp_add 1+1");
  if (dp_sub (3, 2) != 1) fail ("dp_sub 3-2");
  if (dp_mul (2, 3) != 6) fail ("dp_mul 2*3");
  if (dp_div (3, 2) != 1.5) fail ("dp_div 3/2");
  if (dp_neg (1) != -1) fail ("dp_neg 1");
  if (fp_to_dp (1.5) != 1.5) fail ("fp_to_dp 1.5");
  if (dp_to_fp (1.5) != 1.5) fail ("dp_to_fp 1.5");
  if (floatsisf (1) != 1) fail ("floatsisf 1");
  if (floatsidf (1) != 1) fail ("floatsidf 1");
  if (fixsfsi (1.42) != 1) fail ("fixsfsi 1.42");
  if (fixunssfsi (1.42) != 1) fail ("fixunssfsi 1.42");
  if (fixdfsi (1.42) != 1) fail ("fixdfsi 1.42");
  if (fixunsdfsi (1.42) != 1) fail ("fixunsdfsi 1.42");
  if (eqsf2 (1, 1) == 0) fail ("eqsf2 1==1");
  if (eqsf2 (1, 2) != 0) fail ("eqsf2 1==2");
  if (nesf2 (1, 2) == 0) fail ("nesf2 1!=1");
  if (nesf2 (1, 1) != 0) fail ("nesf2 1!=1");
  if (gtsf2 (2, 1) == 0) fail ("gtsf2 2>1");
  if (gtsf2 (1, 1) != 0) fail ("gtsf2 1>1");
  if (gtsf2 (0, 1) != 0) fail ("gtsf2 0>1");
  if (gesf2 (2, 1) == 0) fail ("gesf2 2>=1");
  if (gesf2 (1, 1) == 0) fail ("gesf2 1>=1");
  if (gesf2 (0, 1) != 0) fail ("gesf2 0>=1");
  if (ltsf2 (1, 2) == 0) fail ("ltsf2 1<2");
  if (ltsf2 (1, 1) != 0) fail ("ltsf2 1<1");
  if (ltsf2 (1, 0) != 0) fail ("ltsf2 1<0");
  if (lesf2 (1, 2) == 0) fail ("lesf2 1<=2");
  if (lesf2 (1, 1) == 0) fail ("lesf2 1<=1");
  if (lesf2 (1, 0) != 0) fail ("lesf2 1<=0");
  if (fail_count != 0)
    abort ();
  exit (0);
}
