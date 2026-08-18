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


int div1(signed char x) {

  return x / -1;
}
int div2(signed short x) {

  return x / -1;
}
int div3(signed char x, signed char y) {

  return x / y;
}
int div4(signed short x, signed short y) {

  return x / y;
}
int mod1(signed char x) {

  return x % -1;
}
int mod2(signed short x) {

  return x % -1;
}
int mod3(signed char x, signed char y) {

  return x % y;
}
int mod4(signed short x, signed short y) {

  return x % y;
}

signed long
mod5(signed long x, signed long y) {

  return x % y;
}
     
unsigned long
mod6(unsigned long x, unsigned long y) {

  return x % y;
}
int main() {

  if (div1 (-(1 << 7)) != 1 << 7)
    abort ();
  if (div2 (-(1 << 15)) != 1 << 15)
    abort ();
  if (div3 (-(1 << 7), -1) != 1 << 7)
    abort ();
  if (div4 (-(1 << 15), -1) != 1 << 15)
    abort ();
  if (mod1 (-(1 << 7)) != 0)
    abort ();
  if (mod2 (-(1 << 15)) != 0)
    abort ();
  if (mod3 (-(1 << 7), -1) != 0)
    abort ();
  if (mod4 (-(1 << 15), -1) != 0)
    abort ();
  if (mod5 (0x50000000, 2) != 0)
    abort ();
  if (mod6 (0x50000000, 2) != 0)
    abort ();
  
  exit (0);
  return 0;
}
