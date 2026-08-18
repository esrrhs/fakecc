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
int ops[13] =
{
  11, 12, 46, 3, 2, 2, 3, 2, 1, 3, 2, 1, 2
};
int correct[13] =
{
  46, 12, 11, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1
};
int num = 13;
int main()
{
  int i;
  for (i = 0; i < num; i++)
    {
      int j;
      for (j = num - 1; j > i; j--)
        {
          if (ops[j-1] < ops[j])
            {
              int op = ops[j];
              ops[j] = ops[j-1];
              ops[j-1] = op;
            }
        }
    }
  for (i = 0; i < num; i++)
    if (ops[i] != correct[i])
      abort ();
  exit (0);
}
