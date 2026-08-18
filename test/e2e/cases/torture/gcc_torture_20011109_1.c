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
void fail1(void)
{
  abort ();
}
void fail2(void)
{
  abort ();
}
void fail3(void)
{
  abort ();
}
void fail4(void)
{
  abort ();
}
void foo(long x)
{
  switch (x)
    {
    case -6:
      fail1 (); break;
    case 0:
      fail2 (); break;
    case 1: case 2:
      break;
    case 3: case 4: case 5:
      fail3 ();
      break;
    default:
      fail4 ();
      break;
    }
  
switch (x)
    {
    case -3:
      fail1 (); break;
    case 0: case 4:
      fail2 (); break;
    case 1: case 3:
      break;
    case 2: case 8:
      abort ();
      break;
    default:
      fail4 ();
      break;
    }
}
int main(void)
{
  foo (1);
  exit (0);
}

