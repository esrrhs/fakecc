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
unsigned char uc = ((unsigned char)0xf234U);
unsigned short us = ((unsigned short)0xf234U);
unsigned int ui = 0xf234U;
unsigned long ul = 0xf2345678LU;
unsigned long long ull = 0xf2345678abcdef0LLU;
int shift1 = 4;
int shift2 = ((sizeof (long long) * 8) - 4);
int main(void)
{
  if ((((uc) >> (shift1)) | ((uc) << ((sizeof (uc) * 8) - (shift1)))) != (((((unsigned char)0xf234U)) >> (4)) | ((((unsigned char)0xf234U)) << ((sizeof (((unsigned char)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((uc) >> (4)) | ((uc) << ((sizeof (uc) * 8) - (4)))) != (((((unsigned char)0xf234U)) >> (4)) | ((((unsigned char)0xf234U)) << ((sizeof (((unsigned char)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((us) >> (shift1)) | ((us) << ((sizeof (us) * 8) - (shift1)))) != (((((unsigned short)0xf234U)) >> (4)) | ((((unsigned short)0xf234U)) << ((sizeof (((unsigned short)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((us) >> (4)) | ((us) << ((sizeof (us) * 8) - (4)))) != (((((unsigned short)0xf234U)) >> (4)) | ((((unsigned short)0xf234U)) << ((sizeof (((unsigned short)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((ui) >> (shift1)) | ((ui) << ((sizeof (ui) * 8) - (shift1)))) != (((0xf234U) >> (4)) | ((0xf234U) << ((sizeof (0xf234U) * 8) - (4)))))
    abort ();
  if ((((ui) >> (4)) | ((ui) << ((sizeof (ui) * 8) - (4)))) != (((0xf234U) >> (4)) | ((0xf234U) << ((sizeof (0xf234U) * 8) - (4)))))
    abort ();
  if ((((ul) >> (shift1)) | ((ul) << ((sizeof (ul) * 8) - (shift1)))) != (((0xf2345678LU) >> (4)) | ((0xf2345678LU) << ((sizeof (0xf2345678LU) * 8) - (4)))))
    abort ();
  if ((((ul) >> (4)) | ((ul) << ((sizeof (ul) * 8) - (4)))) != (((0xf2345678LU) >> (4)) | ((0xf2345678LU) << ((sizeof (0xf2345678LU) * 8) - (4)))))
    abort ();
  if ((((ull) >> (shift1)) | ((ull) << ((sizeof (ull) * 8) - (shift1)))) != (((0xf2345678abcdef0LLU) >> (4)) | ((0xf2345678abcdef0LLU) << ((sizeof (0xf2345678abcdef0LLU) * 8) - (4)))))
    abort ();
  if ((((ull) >> (4)) | ((ull) << ((sizeof (ull) * 8) - (4)))) != (((0xf2345678abcdef0LLU) >> (4)) | ((0xf2345678abcdef0LLU) << ((sizeof (0xf2345678abcdef0LLU) * 8) - (4)))))
    abort ();
  if ((((ull) >> (shift2)) | ((ull) << ((sizeof (ull) * 8) - (shift2)))) != (((0xf2345678abcdef0LLU) >> (((sizeof (long long) * 8) - 4))) | ((0xf2345678abcdef0LLU) << ((sizeof (0xf2345678abcdef0LLU) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((ull) >> (((sizeof (long long) * 8) - 4))) | ((ull) << ((sizeof (ull) * 8) - (((sizeof (long long) * 8) - 4))))) != (((0xf2345678abcdef0LLU) >> (((sizeof (long long) * 8) - 4))) | ((0xf2345678abcdef0LLU) << ((sizeof (0xf2345678abcdef0LLU) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((uc) << (shift1)) | ((uc) >> ((sizeof (uc) * 8) - (shift1)))) != (((((unsigned char)0xf234U)) << (4)) | ((((unsigned char)0xf234U)) >> ((sizeof (((unsigned char)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((uc) << (4)) | ((uc) >> ((sizeof (uc) * 8) - (4)))) != (((((unsigned char)0xf234U)) << (4)) | ((((unsigned char)0xf234U)) >> ((sizeof (((unsigned char)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((us) << (shift1)) | ((us) >> ((sizeof (us) * 8) - (shift1)))) != (((((unsigned short)0xf234U)) << (4)) | ((((unsigned short)0xf234U)) >> ((sizeof (((unsigned short)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((us) << (4)) | ((us) >> ((sizeof (us) * 8) - (4)))) != (((((unsigned short)0xf234U)) << (4)) | ((((unsigned short)0xf234U)) >> ((sizeof (((unsigned short)0xf234U)) * 8) - (4)))))
    abort ();
  if ((((ui) << (shift1)) | ((ui) >> ((sizeof (ui) * 8) - (shift1)))) != (((0xf234U) << (4)) | ((0xf234U) >> ((sizeof (0xf234U) * 8) - (4)))))
    abort ();
  if ((((ui) << (4)) | ((ui) >> ((sizeof (ui) * 8) - (4)))) != (((0xf234U) << (4)) | ((0xf234U) >> ((sizeof (0xf234U) * 8) - (4)))))
    abort ();
  if ((((ul) << (shift1)) | ((ul) >> ((sizeof (ul) * 8) - (shift1)))) != (((0xf2345678LU) << (4)) | ((0xf2345678LU) >> ((sizeof (0xf2345678LU) * 8) - (4)))))
    abort ();
  if ((((ul) << (4)) | ((ul) >> ((sizeof (ul) * 8) - (4)))) != (((0xf2345678LU) << (4)) | ((0xf2345678LU) >> ((sizeof (0xf2345678LU) * 8) - (4)))))
    abort ();
  if ((((ull) << (shift1)) | ((ull) >> ((sizeof (ull) * 8) - (shift1)))) != (((0xf2345678abcdef0LLU) << (4)) | ((0xf2345678abcdef0LLU) >> ((sizeof (0xf2345678abcdef0LLU) * 8) - (4)))))
    abort ();
  if ((((ull) << (4)) | ((ull) >> ((sizeof (ull) * 8) - (4)))) != (((0xf2345678abcdef0LLU) << (4)) | ((0xf2345678abcdef0LLU) >> ((sizeof (0xf2345678abcdef0LLU) * 8) - (4)))))
    abort ();
  if ((((ull) << (shift2)) | ((ull) >> ((sizeof (ull) * 8) - (shift2)))) != (((0xf2345678abcdef0LLU) << (((sizeof (long long) * 8) - 4))) | ((0xf2345678abcdef0LLU) >> ((sizeof (0xf2345678abcdef0LLU) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((ull) << (((sizeof (long long) * 8) - 4))) | ((ull) >> ((sizeof (ull) * 8) - (((sizeof (long long) * 8) - 4))))) != (((0xf2345678abcdef0LLU) << (((sizeof (long long) * 8) - 4))) | ((0xf2345678abcdef0LLU) >> ((sizeof (0xf2345678abcdef0LLU) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  exit (0);
}

