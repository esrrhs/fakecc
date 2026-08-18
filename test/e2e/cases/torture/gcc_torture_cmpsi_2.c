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

int feq(int x, int y) {
  if (x == y)
    return 13;
  else
    return 140;
}
int fne(int x, int y) {
  if (x != y)
    return 13;
  else
    return 140;
}
int flt(int x, int y) {
  if (x < y)
    return 13;
  else
    return 140;
}
int fge(int x, int y) {
  if (x >= y)
    return 13;
  else
    return 140;
}
int fgt(int x, int y) {
  if (x > y)
    return 13;
  else
    return 140;
}
int fle(int x, int y) {
  if (x <= y)
    return 13;
  else
    return 140;
}
int fltu(unsigned int x, unsigned int y) {
  if (x < y)
    return 13;
  else
    return 140;
}
int fgeu(unsigned int x, unsigned int y) {
  if (x >= y)
    return 13;
  else
    return 140;
}
int fgtu(unsigned int x, unsigned int y) {
  if (x > y)
    return 13;
  else
    return 140;
}
int fleu(unsigned int x, unsigned int y) {
  if (x <= y)
    return 13;
  else
    return 140;
}
unsigned int args[] =
{
  0L,
  1L,
  -1L,
  0x7fffffffL,
  0x80000000L,
  0x80000001L,
  0x1A3F2373L,
  0x93850E92L
};
int correct_results[] =
{
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13,
  140, 13, 140, 13, 13, 140, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 13, 140, 140, 13,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 140, 13, 13, 140, 140, 13, 13, 140,
  140, 13, 13, 140, 140, 13, 140, 13, 13, 140,
  13, 140, 140, 13, 140, 13, 140, 13, 140, 13
};
int main(void)
{
  int i, j, *res = correct_results;
  for (i = 0; i < 8; i++)
    {
      unsigned int arg0 = args[i];
      for (j = 0; j < 8; j++)
 {
   unsigned int arg1 = args[j];
   if (feq (arg0, arg1) != *res++)
     abort ();
   if (fne (arg0, arg1) != *res++)
     abort ();
   if (flt (arg0, arg1) != *res++)
     abort ();
   if (fge (arg0, arg1) != *res++)
     abort ();
   if (fgt (arg0, arg1) != *res++)
     abort ();
   if (fle (arg0, arg1) != *res++)
     abort ();
   if (fltu (arg0, arg1) != *res++)
     abort ();
   if (fgeu (arg0, arg1) != *res++)
     abort ();
   if (fgtu (arg0, arg1) != *res++)
     abort ();
   if (fleu (arg0, arg1) != *res++)
     abort ();
 }
    }
  exit (0);
}
