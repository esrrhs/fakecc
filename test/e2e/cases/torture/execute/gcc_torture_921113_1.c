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
  float wsx;
} struct_list;
typedef struct_list *list_t;
typedef struct {
  float x, y;
} vector_t;
void w(float x, float y) {}
void f1(float x, float y)
{
  if (x != 0 || y != 0)
    abort();
}
void f2(float x, float y)
{
  if (x != 1 || y != 1)
    abort();
}
void gitter(int count, vector_t pos[], list_t list, int *nww, vector_t limit[2], float r)
{
  float d;
  int gitt[128][128];
  f1(limit[0].x, limit[0].y);
  f2(limit[1].x, limit[1].y);
  *nww = 0;
  d = pos[0].x;
  if (d <= 0.)
    {
      w(d, r);
      if (d <= r * 0.5)
 {
   w(d, r);
   list[0].wsx = 1;
 }
    }
}
vector_t pos[1] = {{0., 0.}};
vector_t limit[2] = {{0.,0.},{1.,1.}};
int main(void)
{
  int nww;
  struct_list list;
  gitter(1, pos, &list, &nww, limit, 1.);
  exit(0);
}

