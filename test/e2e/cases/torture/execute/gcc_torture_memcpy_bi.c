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
char src[80] ;
char dst[80] ;
void check(char *test, char *match, int n)
{
  if (memcmp (test, match, n))
    abort ();
}
int main(void)
{
  int i,j;
  for (i = 0; i < sizeof (src); ++i)
      src[i] = 'a' + i % 26;
  { memset (dst, 0, 0); memcpy (dst, src, 0); check (dst, src, 0); } { memset (dst, 0, (0) + 1); memcpy (dst, src, (0) + 1); check (dst, src, (0) + 1); } { memset (dst, 0, (0) + 2); memcpy (dst, src, (0) + 2); check (dst, src, (0) + 2); } { memset (dst, 0, (0) + 3); memcpy (dst, src, (0) + 3); check (dst, src, (0) + 3); };
  { memset (dst, 0, 4); memcpy (dst, src, 4); check (dst, src, 4); } { memset (dst, 0, (4) + 1); memcpy (dst, src, (4) + 1); check (dst, src, (4) + 1); } { memset (dst, 0, (4) + 2); memcpy (dst, src, (4) + 2); check (dst, src, (4) + 2); } { memset (dst, 0, (4) + 3); memcpy (dst, src, (4) + 3); check (dst, src, (4) + 3); };
  { memset (dst, 0, 8); memcpy (dst, src, 8); check (dst, src, 8); } { memset (dst, 0, (8) + 1); memcpy (dst, src, (8) + 1); check (dst, src, (8) + 1); } { memset (dst, 0, (8) + 2); memcpy (dst, src, (8) + 2); check (dst, src, (8) + 2); } { memset (dst, 0, (8) + 3); memcpy (dst, src, (8) + 3); check (dst, src, (8) + 3); };
  { memset (dst, 0, 12); memcpy (dst, src, 12); check (dst, src, 12); } { memset (dst, 0, (12) + 1); memcpy (dst, src, (12) + 1); check (dst, src, (12) + 1); } { memset (dst, 0, (12) + 2); memcpy (dst, src, (12) + 2); check (dst, src, (12) + 2); } { memset (dst, 0, (12) + 3); memcpy (dst, src, (12) + 3); check (dst, src, (12) + 3); };
  { memset (dst, 0, 16); memcpy (dst, src, 16); check (dst, src, 16); } { memset (dst, 0, (16) + 1); memcpy (dst, src, (16) + 1); check (dst, src, (16) + 1); } { memset (dst, 0, (16) + 2); memcpy (dst, src, (16) + 2); check (dst, src, (16) + 2); } { memset (dst, 0, (16) + 3); memcpy (dst, src, (16) + 3); check (dst, src, (16) + 3); };
  { memset (dst, 0, 20); memcpy (dst, src, 20); check (dst, src, 20); } { memset (dst, 0, (20) + 1); memcpy (dst, src, (20) + 1); check (dst, src, (20) + 1); } { memset (dst, 0, (20) + 2); memcpy (dst, src, (20) + 2); check (dst, src, (20) + 2); } { memset (dst, 0, (20) + 3); memcpy (dst, src, (20) + 3); check (dst, src, (20) + 3); };
  { memset (dst, 0, 24); memcpy (dst, src, 24); check (dst, src, 24); } { memset (dst, 0, (24) + 1); memcpy (dst, src, (24) + 1); check (dst, src, (24) + 1); } { memset (dst, 0, (24) + 2); memcpy (dst, src, (24) + 2); check (dst, src, (24) + 2); } { memset (dst, 0, (24) + 3); memcpy (dst, src, (24) + 3); check (dst, src, (24) + 3); };
  { memset (dst, 0, 28); memcpy (dst, src, 28); check (dst, src, 28); } { memset (dst, 0, (28) + 1); memcpy (dst, src, (28) + 1); check (dst, src, (28) + 1); } { memset (dst, 0, (28) + 2); memcpy (dst, src, (28) + 2); check (dst, src, (28) + 2); } { memset (dst, 0, (28) + 3); memcpy (dst, src, (28) + 3); check (dst, src, (28) + 3); };
  { memset (dst, 0, 32); memcpy (dst, src, 32); check (dst, src, 32); } { memset (dst, 0, (32) + 1); memcpy (dst, src, (32) + 1); check (dst, src, (32) + 1); } { memset (dst, 0, (32) + 2); memcpy (dst, src, (32) + 2); check (dst, src, (32) + 2); } { memset (dst, 0, (32) + 3); memcpy (dst, src, (32) + 3); check (dst, src, (32) + 3); };
  { memset (dst, 0, 36); memcpy (dst, src, 36); check (dst, src, 36); } { memset (dst, 0, (36) + 1); memcpy (dst, src, (36) + 1); check (dst, src, (36) + 1); } { memset (dst, 0, (36) + 2); memcpy (dst, src, (36) + 2); check (dst, src, (36) + 2); } { memset (dst, 0, (36) + 3); memcpy (dst, src, (36) + 3); check (dst, src, (36) + 3); };
  { memset (dst, 0, 40); memcpy (dst, src, 40); check (dst, src, 40); } { memset (dst, 0, (40) + 1); memcpy (dst, src, (40) + 1); check (dst, src, (40) + 1); } { memset (dst, 0, (40) + 2); memcpy (dst, src, (40) + 2); check (dst, src, (40) + 2); } { memset (dst, 0, (40) + 3); memcpy (dst, src, (40) + 3); check (dst, src, (40) + 3); };
  { memset (dst, 0, 44); memcpy (dst, src, 44); check (dst, src, 44); } { memset (dst, 0, (44) + 1); memcpy (dst, src, (44) + 1); check (dst, src, (44) + 1); } { memset (dst, 0, (44) + 2); memcpy (dst, src, (44) + 2); check (dst, src, (44) + 2); } { memset (dst, 0, (44) + 3); memcpy (dst, src, (44) + 3); check (dst, src, (44) + 3); };
  { memset (dst, 0, 48); memcpy (dst, src, 48); check (dst, src, 48); } { memset (dst, 0, (48) + 1); memcpy (dst, src, (48) + 1); check (dst, src, (48) + 1); } { memset (dst, 0, (48) + 2); memcpy (dst, src, (48) + 2); check (dst, src, (48) + 2); } { memset (dst, 0, (48) + 3); memcpy (dst, src, (48) + 3); check (dst, src, (48) + 3); };
  { memset (dst, 0, 52); memcpy (dst, src, 52); check (dst, src, 52); } { memset (dst, 0, (52) + 1); memcpy (dst, src, (52) + 1); check (dst, src, (52) + 1); } { memset (dst, 0, (52) + 2); memcpy (dst, src, (52) + 2); check (dst, src, (52) + 2); } { memset (dst, 0, (52) + 3); memcpy (dst, src, (52) + 3); check (dst, src, (52) + 3); };
  { memset (dst, 0, 56); memcpy (dst, src, 56); check (dst, src, 56); } { memset (dst, 0, (56) + 1); memcpy (dst, src, (56) + 1); check (dst, src, (56) + 1); } { memset (dst, 0, (56) + 2); memcpy (dst, src, (56) + 2); check (dst, src, (56) + 2); } { memset (dst, 0, (56) + 3); memcpy (dst, src, (56) + 3); check (dst, src, (56) + 3); };
  { memset (dst, 0, 60); memcpy (dst, src, 60); check (dst, src, 60); } { memset (dst, 0, (60) + 1); memcpy (dst, src, (60) + 1); check (dst, src, (60) + 1); } { memset (dst, 0, (60) + 2); memcpy (dst, src, (60) + 2); check (dst, src, (60) + 2); } { memset (dst, 0, (60) + 3); memcpy (dst, src, (60) + 3); check (dst, src, (60) + 3); };
  { memset (dst, 0, 64); memcpy (dst, src, 64); check (dst, src, 64); } { memset (dst, 0, (64) + 1); memcpy (dst, src, (64) + 1); check (dst, src, (64) + 1); } { memset (dst, 0, (64) + 2); memcpy (dst, src, (64) + 2); check (dst, src, (64) + 2); } { memset (dst, 0, (64) + 3); memcpy (dst, src, (64) + 3); check (dst, src, (64) + 3); };
  { memset (dst, 0, 68); memcpy (dst, src, 68); check (dst, src, 68); } { memset (dst, 0, (68) + 1); memcpy (dst, src, (68) + 1); check (dst, src, (68) + 1); } { memset (dst, 0, (68) + 2); memcpy (dst, src, (68) + 2); check (dst, src, (68) + 2); } { memset (dst, 0, (68) + 3); memcpy (dst, src, (68) + 3); check (dst, src, (68) + 3); };
  { memset (dst, 0, 72); memcpy (dst, src, 72); check (dst, src, 72); } { memset (dst, 0, (72) + 1); memcpy (dst, src, (72) + 1); check (dst, src, (72) + 1); } { memset (dst, 0, (72) + 2); memcpy (dst, src, (72) + 2); check (dst, src, (72) + 2); } { memset (dst, 0, (72) + 3); memcpy (dst, src, (72) + 3); check (dst, src, (72) + 3); };
  { memset (dst, 0, 76); memcpy (dst, src, 76); check (dst, src, 76); } { memset (dst, 0, (76) + 1); memcpy (dst, src, (76) + 1); check (dst, src, (76) + 1); } { memset (dst, 0, (76) + 2); memcpy (dst, src, (76) + 2); check (dst, src, (76) + 2); } { memset (dst, 0, (76) + 3); memcpy (dst, src, (76) + 3); check (dst, src, (76) + 3); };
  return 0;
}

