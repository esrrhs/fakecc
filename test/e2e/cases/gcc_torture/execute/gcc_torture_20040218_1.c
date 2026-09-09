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
long int xb (long int *y) ;
long int xw (long int *y) ;
short int yb (short int *y) ;
long int xb(long int *y)
{
  long int xx = *y & 255;
  return xx + y[1];
}
long int xw(long int *y)
{
  long int xx = *y & 65535;
  return xx + y[1];
}
short int yb(short int *y)
{
  short int xx = *y & 255;
  return xx + y[1];
}
int main(void)
{
  long int y[] = {-1, 16000};
  short int yw[] = {-1, 16000};
  if (xb (y) != 16255
      || xw (y) != 81535
      || yb (yw) != 16255)
    abort ();
  exit (0);
}

