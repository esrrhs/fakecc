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

void abort(void);
void exit(int);
void test1(void)
{
  int x = 3, y = 2;
  if ((x < y ? x++ : y++) != 2)
    abort ();
  if (x != 3)
    abort ();
  if (y != 3)
    abort ();
}
void test2(void)
{
  int x = 3, y = 2, z;
  z = (x < y) ? x++ : y++;
  if (z != 2)
    abort ();
  if (x != 3)
    abort ();
  if (y != 3)
    abort ();
}
void test3(void)
{
  int x = 3, y = 2;
  int xx = 3, yy = 2;
  if ((xx < yy ? x++ : y++) != 2)
    abort ();
  if (x != 3)
    abort ();
  if (y != 3)
    abort ();
}
int x, y;
static void init_xy(void)
{
  x = 3;
  y = 2;
}
void test4(void)
{
  init_xy();
  if ((x < y ? x++ : y++) != 2)
    abort ();
  if (x != 3)
    abort ();
  if (y != 3)
    abort ();
}
void test5(void)
{
  int z;
  init_xy();
  z = (x < y) ? x++ : y++;
  if (z != 2)
    abort ();
  if (x != 3)
    abort ();
  if (y != 3)
    abort ();
}
void test6(void)
{
  int xx = 3, yy = 2;
  int z;
  init_xy();
  z = (xx < y) ? x++ : y++;
  if (z != 2)
    abort ();
  if (x != 3)
    abort ();
  if (y != 3)
    abort ();
}
int main() {
  test1 ();
  test2 ();
  test3 ();
  test4 ();
  test5 ();
  test6 ();
  exit (0);
}

