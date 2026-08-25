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
} u;
char A = 'A';
int main(void)
{
  int off, len, i;
  char *p, *q;
  for (off = 0; off < (sizeof (long long)); off++)
    for (len = 1; len < (10 * sizeof (long long)); len++)
      {
 for (i = 0; i < ((sizeof (long long)) + (10 * sizeof (long long)) + (sizeof (long long))); i++)
   u.buf[i] = 'a';
 p = memset (u.buf + off, '\0', len);
 if (p != u.buf + off)
   abort ();
 q = u.buf;
 for (i = 0; i < off; i++, q++)
   if (*q != 'a')
     abort ();
 for (i = 0; i < len; i++, q++)
   if (*q != '\0')
     abort ();
 for (i = 0; i < (sizeof (long long)); i++, q++)
   if (*q != 'a')
     abort ();
 p = memset (u.buf + off, A, len);
 if (p != u.buf + off)
   abort ();
 q = u.buf;
 for (i = 0; i < off; i++, q++)
   if (*q != 'a')
     abort ();
 for (i = 0; i < len; i++, q++)
   if (*q != 'A')
     abort ();
 for (i = 0; i < (sizeof (long long)); i++, q++)
   if (*q != 'a')
     abort ();
 p = memset (u.buf + off, 'B', len);
 if (p != u.buf + off)
   abort ();
 q = u.buf;
 for (i = 0; i < off; i++, q++)
   if (*q != 'a')
     abort ();
 for (i = 0; i < len; i++, q++)
   if (*q != 'B')
     abort ();
 for (i = 0; i < (sizeof (long long)); i++, q++)
   if (*q != 'a')
     abort ();
      }
  
exit (0);
}

