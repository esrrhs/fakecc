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

extern void abort (void);
extern void exit (int);
int dd (int x, int d) { return x / d; }
int
main ()
{
  int i;
  for (i = -3; i <= 3; i++)
    {
      if (dd (i, 1) != i / 1)
 abort ();
      if (dd (i, 2) != i / 2)
 abort ();
      if (dd (i, 3) != i / 3)
 abort ();
      if (dd (i, 4) != i / 4)
 abort ();
      if (dd (i, 5) != i / 5)
 abort ();
      if (dd (i, 6) != i / 6)
 abort ();
      if (dd (i, 7) != i / 7)
 abort ();
      if (dd (i, 8) != i / 8)
 abort ();
    }
  for (i = ((unsigned) ~0 >> 1) - 3; i <= ((unsigned) ~0 >> 1) + 3; i++)
    {
      if (dd (i, 1) != i / 1)
 abort ();
      if (dd (i, 2) != i / 2)
 abort ();
      if (dd (i, 3) != i / 3)
 abort ();
      if (dd (i, 4) != i / 4)
 abort ();
      if (dd (i, 5) != i / 5)
 abort ();
      if (dd (i, 6) != i / 6)
 abort ();
      if (dd (i, 7) != i / 7)
 abort ();
      if (dd (i, 8) != i / 8)
 abort ();
    }
  exit (0);
}
