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

extern void exit (int);
extern void abort (void);
volatile int i;
unsigned char *volatile cp;
unsigned char d[32] = { 0 };
int main(void)
{
  unsigned char c[32] = { 0 };
  unsigned char *p = d + i;
  int j;
  for (j = 0; j < 30; j++)
    {
      int x = 0xff;
      int y = *++p;
      switch (j)
 {
 case 1: x ^= 2; break;
 case 2: x ^= 4; break;
 case 25: x ^= 1; break;
 default: break;
 }
      c[j] = y | x;
      cp = p;
    }
  
if (c[0] != 0xff
      || c[1] != 0xfd
      || c[2] != 0xfb
      || c[3] != 0xff
      || c[4] != 0xff
      || c[25] != 0xfe
      || cp != d + 30)
    abort ();
  exit (0);
}

