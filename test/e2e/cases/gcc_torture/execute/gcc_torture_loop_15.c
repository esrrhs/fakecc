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
void
foo (unsigned long *start, unsigned long *end)
{
  unsigned long *temp = end - 1;
  while (end > start)
    *end-- = *temp--;
}
int
main (void)
{
  unsigned long a[5];
  int start, end, k;
  for (start = 0; start < 5; start++)
    for (end = 0; end < 5; end++)
      {
 for (k = 0; k < 5; k++)
   a[k] = k;
 foo (a + start, a + end);
 for (k = 0; k <= start; k++)
   if (a[k] != k)
     abort ();
 for (k = start + 1; k <= end; k++)
   if (a[k] != k - 1)
     abort ();
 for (k = end + 1; k < 5; k++)
   if (a[k] != k)
     abort ();
      }
  return 0;
}
