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

typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef int wchar_t;
char ** buildargv(char *input)
{
  static char *arglist[256];
  int numargs = 0;
  while (1)
    {
      while (*input == ' ')
 input++;
      if (*input == 0)
 break;
      arglist [numargs++] = input;
      while (*input != ' ' && *input != 0)
 input++;
      if (*input == 0)
 break;
      *(input++) = 0;
    }
  arglist [numargs] = ((void*)0);
  return arglist;
}
int main()
{
  char **args;
  char input[256];
  int i;
  strcpy(input, " a b");
  args = buildargv(input);
  if (strcmp (args[0], "a"))
    abort ();
  if (strcmp (args[1], "b"))
    abort ();
  if (args[2] != ((void*)0))
    abort ();
  exit (0);
}

