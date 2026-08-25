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



__extension__ typedef long ptr_t;
ptr_t *wm_TR;
ptr_t *wm_HB;
ptr_t *wm_SPB;

ptr_t mem[100];

int f(ptr_t *mr_TR, ptr_t *mr_SPB, ptr_t *mr_HB, ptr_t *reg1, ptr_t *reg2) {
  ptr_t *x = mr_TR;

  for (;;)
    {
      if (reg1 < reg2)
	goto out;
      if ((ptr_t *) *reg1 < mr_HB && (ptr_t *) *reg1 >= mr_SPB)
	*--mr_TR = *reg1;
      reg1--;
    }
 out:

  if (x != mr_TR)
    abort ();
}

int main() {
  mem[99] = (ptr_t) mem;
  f (mem + 100, mem + 6, mem + 8, mem + 99, mem + 99);
  exit (0);
}
