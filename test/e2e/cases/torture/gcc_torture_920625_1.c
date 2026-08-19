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

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];

void abort (void);
void exit (int);

typedef struct{double x,y;}point;
point pts[]={{1.0,2.0},{3.0,4.0},{5.0,6.0},{7.0,8.0}};
static int va1(int nargs,...)
{
  va_list args;
  int i;
  point pi;
  va_start(args,nargs);
  for(i=0;i<nargs;i++){
    pi=va_arg(args,point);
    if(pts[i].x!=pi.x||pts[i].y!=pi.y)abort();
  }
  va_end(args);
}

typedef struct{int x,y;}ipoint;
ipoint ipts[]={{1,2},{3,4},{5,6},{7,8}};
static int va2(int nargs,...)
{
  va_list args;
  int i;
  ipoint pi;
  va_start(args,nargs);
  for(i=0;i<nargs;i++){
    pi=va_arg(args,ipoint);
    if(ipts[i].x!=pi.x||ipts[i].y!=pi.y)abort();
  }
  va_end(args);
}

int
main(void)
{
va1(4,pts[0],pts[1],pts[2],pts[3]);
va2(4,ipts[0],ipts[1],ipts[2],ipts[3]);
exit(0);
}
