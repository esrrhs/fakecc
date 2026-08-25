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

void abort (void);
void exit (int);

short optab[5];
char buf[10];
void
execute (ip)
     register short *ip;
{
  register void *base = &&x;
  char *bp = buf;
  static void *tab[] = {&&x, &&y, &&z};
  if (ip == 0)
    {
      int i;
      for (i = 0; i < 3; ++i)
	optab[i] = (short)(tab[i] - base);
      return;
    }
x:  *bp++='x';
    goto *(base + *ip++);
y:  *bp++='y';
    goto *(base + *ip++);
z:  *bp++='z';
    *bp=0;
    return;
}

short p[5];

int
main (void)
{
  execute ((short *) 0);
  p[0] = optab[1];
  p[1] = optab[0];
  p[2] = optab[1];
  p[3] = optab[2];
  execute (&p);
  if (__builtin_strcmp (buf, "xyxyz"))
    abort ();
  exit (0);
}
