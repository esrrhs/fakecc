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
typedef struct {
  long int p_x, p_y;
} Point;
void bar()
{
}
void f(Point p0, Point p1, Point p2, Point p3, Point p4, Point p5) {

  if (p0.p_x != 0 || p0.p_y != 1
      || p1.p_x != -1 || p1.p_y != 0
      || p2.p_x != 1 || p2.p_y != -1
      || p3.p_x != -1 || p3.p_y != 1
      || p4.p_x != 0 || p4.p_y != -1
      || p5.p_x != 1 || p5.p_y != 0)
    abort ();
}
void foo()
{
  Point p0, p1, p2, p3, p4, p5;
  bar();
  p0.p_x = 0;
  p0.p_y = 1;
  p1.p_x = -1;
  p1.p_y = 0;
  p2.p_x = 1;
  p2.p_y = -1;
  p3.p_x = -1;
  p3.p_y = 1;
  p4.p_x = 0;
  p4.p_y = -1;
  p5.p_x = 1;
  p5.p_y = 0;
  f (p0, p1, p2, p3, p4, p5);
}
int main()
{
  foo();
  exit(0);
}

