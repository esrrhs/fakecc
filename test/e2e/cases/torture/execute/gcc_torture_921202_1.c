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
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
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
extern int isprint(int);

enum { VLEN = 2055 };

main ()
{
  long dx[VLEN+1];
  long dy[VLEN+1];
  long s1[VLEN];
  int cyx, cyy;
  int i;
  long size;

  for (;;)
    {
      size = VLEN;
      mpn_random2 (s1, size);

      for (i = 0; i < 1; i++)
	;

      dy[size] = 0x12345678;

      for (i = 0; i < 1; i++)
	cyy = mpn_mul_1 (dy, s1, size);

      if (cyx != cyy || mpn_cmp (dx, dy, size + 1) != 0 || dx[size] != 0x12345678)
	{
	  foo ("", 8, cyy); mpn_print (dy, size);
	}
      exxit();
    }
}

foo (){}
mpn_mul_1(){}
mpn_print (){}
mpn_random2(){}
mpn_cmp(){}
exxit(){exit(0);}
