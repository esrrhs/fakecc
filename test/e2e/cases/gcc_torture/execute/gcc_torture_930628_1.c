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
void
f (double x[2], double y[2])
{
  if (x == y)
    abort ();
}
int
main (void)
{
  struct { int f[3]; double x[1][2]; } tp[4][2];
  int i, j, ki, kj, mi, mj;
  float bdm[4][2][4][2];
  for (i = 0; i < 4; i++)
    for (j = i; j < 4; j++)
      for (ki = 0; ki < 2; ki++)
 for (kj = 0; kj < 2; kj++)
   if ((j == i) && (ki == kj))
     bdm[i][ki][j][kj] = 1000.0;
   else
     {
       for (mi = 0; mi < 1; mi++)
  for (mj = 0; mj < 1; mj++)
    f (tp[i][ki].x[mi], tp[j][kj].x[mj]);
       bdm[i][ki][j][kj] = 1000.0;
     }
  exit (0);
}
