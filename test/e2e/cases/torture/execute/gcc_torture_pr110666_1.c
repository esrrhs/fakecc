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

int eqeq_0 (int) ; int eqeq_0 (int a) { return (a == 0) == a; } int eqeq_0_v (int) ; int eqeq_0_v (volatile int a) { return (a == 0) == a; } int eqeq_1 (int) ; int eqeq_1 (int a) { return (a == 1) == a; } int eqeq_1_v (int) ; int eqeq_1_v (volatile int a) { return (a == 1) == a; } int eqeq_2 (int) ; int eqeq_2 (int a) { return (a == 2) == a; } int eqeq_2_v (int) ; int eqeq_2_v (volatile int a) { return (a == 2) == a; } int eqne_0 (int) ; int eqne_0 (int a) { return (a != 0) == a; } int eqne_0_v (int) ; int eqne_0_v (volatile int a) { return (a != 0) == a; } int eqne_1 (int) ; int eqne_1 (int a) { return (a != 1) == a; } int eqne_1_v (int) ; int eqne_1_v (volatile int a) { return (a != 1) == a; } int eqne_2 (int) ; int eqne_2 (int a) { return (a != 2) == a; } int eqne_2_v (int) ; int eqne_2_v (volatile int a) { return (a != 2) == a; } int neeq_0 (int) ; int neeq_0 (int a) { return (a == 0) != a; } int neeq_0_v (int) ; int neeq_0_v (volatile int a) { return (a == 0) != a; } int neeq_1 (int) ; int neeq_1 (int a) { return (a == 1) != a; } int neeq_1_v (int) ; int neeq_1_v (volatile int a) { return (a == 1) != a; } int neeq_2 (int) ; int neeq_2 (int a) { return (a == 2) != a; } int neeq_2_v (int) ; int neeq_2_v (volatile int a) { return (a == 2) != a; } int nene_0 (int) ; int nene_0 (int a) { return (a != 0) != a; } int nene_0_v (int) ; int nene_0_v (volatile int a) { return (a != 0) != a; } int nene_1 (int) ; int nene_1 (int a) { return (a != 1) != a; } int nene_1_v (int) ; int nene_1_v (volatile int a) { return (a != 1) != a; } int nene_2 (int) ; int nene_2 (int a) { return (a != 2) != a; } int nene_2_v (int) ; int nene_2_v (volatile int a) { return (a != 2) != a; }
int main()
{
  for(int n = -1; n <= 2; n++) {
    if (eqeq_0_v(n) != eqeq_0(n)) abort(); if (eqeq_1_v(n) != eqeq_1(n)) abort(); if (eqeq_2_v(n) != eqeq_2(n)) abort(); if (eqne_0_v(n) != eqne_0(n)) abort(); if (eqne_1_v(n) != eqne_1(n)) abort(); if (eqne_2_v(n) != eqne_2(n)) abort(); if (neeq_0_v(n) != neeq_0(n)) abort(); if (neeq_1_v(n) != neeq_1(n)) abort(); if (neeq_2_v(n) != neeq_2(n)) abort(); if (nene_0_v(n) != nene_0(n)) abort(); if (nene_1_v(n) != nene_1(n)) abort(); if (nene_2_v(n) != nene_2(n)) abort();
  }
}

