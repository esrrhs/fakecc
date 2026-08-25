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
void abort (void);
void exit (int);
static union {
  char buf[((sizeof (long long)) + (10 * sizeof (long long)) + (sizeof (long long)))];
  long long align_int;
  long double align_fp;
} u1, u2;
int main(void)
{
  int off1, off2, len, i;
  char *p, *q, c;
  for (off1 = 0; off1 < (sizeof (long long)); off1++)
    for (off2 = 0; off2 < (sizeof (long long)); off2++)
      for (len = 1; len < (10 * sizeof (long long)); len++)
 {
   for (i = 0, c = 'A'; i < ((sizeof (long long)) + (10 * sizeof (long long)) + (sizeof (long long))); i++, c++)
     {
       u1.buf[i] = 'a';
       if (c >= 'A' + 31)
  c = 'A';
       u2.buf[i] = c;
     }
   p = memcpy (u1.buf + off1, u2.buf + off2, len);
   if (p != u1.buf + off1)
     abort ();
   q = u1.buf;
   for (i = 0; i < off1; i++, q++)
     if (*q != 'a')
       abort ();
   for (i = 0, c = 'A' + off2; i < len; i++, q++, c++)
     {
       if (c >= 'A' + 31)
  c = 'A';
       if (*q != c)
  abort ();
     }
   
for (i = 0; i < (sizeof (long long)); i++, q++)
     if (*q != 'a')
       abort ();
 }
  
exit (0);
}

