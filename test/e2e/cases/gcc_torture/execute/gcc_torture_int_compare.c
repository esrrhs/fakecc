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

static const int INT_MAX = 2147483647;
static const int INT_MIN = (-2147483647 - 1);

int gt(int a, int b) {

  return a > b;
}
int ge(int a, int b) {

  return a >= b;
}
int lt(int a, int b) {

  return a < b;
}
int le(int a, int b) {

  return a <= b;
}

void
true(int c) {

  if (!c)
    abort();
}

void
false(int c) {

  if (c)
    abort();
}
int f() {

  true (gt (2, 1));
  false (gt (1, 2));

  true (gt (INT_MAX, 0));
  false (gt (0, INT_MAX));
  true (gt (INT_MAX, 1));
  false (gt (1, INT_MAX));

  false (gt (INT_MIN, 0));
  true (gt (0, INT_MIN));
  false (gt (INT_MIN, 1));
  true (gt (1, INT_MIN));

  true (gt (INT_MAX, INT_MIN));
  false (gt (INT_MIN, INT_MAX));

  true (ge (2, 1));
  false (ge (1, 2));

  true (ge (INT_MAX, 0));
  false (ge (0, INT_MAX));
  true (ge (INT_MAX, 1));
  false (ge (1, INT_MAX));

  false (ge (INT_MIN, 0));
  true (ge (0, INT_MIN));
  false (ge (INT_MIN, 1));
  true (ge (1, INT_MIN));

  true (ge (INT_MAX, INT_MIN));
  false (ge (INT_MIN, INT_MAX));

  false (lt (2, 1));
  true (lt (1, 2));

  false (lt (INT_MAX, 0));
  true (lt (0, INT_MAX));
  false (lt (INT_MAX, 1));
  true (lt (1, INT_MAX));

  true (lt (INT_MIN, 0));
  false (lt (0, INT_MIN));
  true (lt (INT_MIN, 1));
  false (lt (1, INT_MIN));

  false (lt (INT_MAX, INT_MIN));
  true (lt (INT_MIN, INT_MAX));

  false (le (2, 1));
  true (le (1, 2));

  false (le (INT_MAX, 0));
  true (le (0, INT_MAX));
  false (le (INT_MAX, 1));
  true (le (1, INT_MAX));

  true (le (INT_MIN, 0));
  false (le (0, INT_MIN));
  true (le (INT_MIN, 1));
  false (le (1, INT_MIN));

  false (le (INT_MAX, INT_MIN));
  true (le (INT_MIN, INT_MAX));
  return 0;
}
int main() {

  f ();
  exit (0);
  return 0;
}
