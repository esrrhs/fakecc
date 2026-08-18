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
int
f (Point basePt, Point pt1, Point pt2)
{
  long long vector;
  vector =
    (long long) (pt1.p_x - basePt.p_x) * (long long) (pt2.p_y - basePt.p_y) -
      (long long) (pt1.p_y - basePt.p_y) * (long long) (pt2.p_x - basePt.p_x);
  if (vector > (long long) 0)
    return 0;
  else if (vector < (long long) 0)
    return 1;
  else
    return 2;
}
int
main (void)
{
  Point b, p1, p2;
  int answer;
  b.p_x = -23250;
  b.p_y = 23250;
  p1.p_x = 23250;
  p1.p_y = -23250;
  p2.p_x = -23250;
  p2.p_y = -23250;
  answer = f (b, p1, p2);
  if (answer != 1)
    abort ();
  exit (0);
}
