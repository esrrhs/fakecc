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
int i;
double d;
float rootbeer[__builtin_types_compatible_p (int, typeof(i))];
typedef enum { hot, dog, poo, bear } dingos;
typedef enum { janette, laura, amanda } cranberry;
typedef float same1;
typedef float same2;
int main (void);
int main (void)
{
  if (!(__builtin_types_compatible_p (int, const int)
 && __builtin_types_compatible_p (typeof (hot), int)
 && __builtin_types_compatible_p (typeof (hot), typeof (laura))
 && __builtin_types_compatible_p (int[5], int[])
 && __builtin_types_compatible_p (same1, same2)))
    abort ();
  if (__builtin_types_compatible_p (char *, int)
      || __builtin_types_compatible_p (char *, const char *)
      || __builtin_types_compatible_p (long double, double)
      || __builtin_types_compatible_p (typeof (i), typeof (d))
      || __builtin_types_compatible_p (typeof (dingos), typeof (cranberry))
      || __builtin_types_compatible_p (char, int)
      || __builtin_types_compatible_p (char *, char **))
    abort ();
  exit (0);
}
