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
char c = ((char)0xf234);
short s = ((short)0xf234);
int i = ((int)0xf234);
long l = ((long)0xf2345678L);
long long ll = ((long long)0xf2345678abcdef0LL);
int shift1 = 4;
int shift2 = ((sizeof (long long) * 8) - 4);
int main(void)
{
  if ((((c) >> (shift1)) | ((c) << ((sizeof (c) * 8) - (shift1)))) != (((((char)0xf234)) >> (4)) | ((((char)0xf234)) << ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((c) >> (4)) | ((c) << ((sizeof (c) * 8) - (4)))) != (((((char)0xf234)) >> (4)) | ((((char)0xf234)) << ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) >> (shift1)) | ((s) << ((sizeof (s) * 8) - (shift1)))) != (((((short)0xf234)) >> (4)) | ((((short)0xf234)) << ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) >> (4)) | ((s) << ((sizeof (s) * 8) - (4)))) != (((((short)0xf234)) >> (4)) | ((((short)0xf234)) << ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) >> (shift1)) | ((i) << ((sizeof (i) * 8) - (shift1)))) != (((((int)0xf234)) >> (4)) | ((((int)0xf234)) << ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) >> (4)) | ((i) << ((sizeof (i) * 8) - (4)))) != (((((int)0xf234)) >> (4)) | ((((int)0xf234)) << ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((l) >> (shift1)) | ((l) << ((sizeof (l) * 8) - (shift1)))) != (((((long)0xf2345678L)) >> (4)) | ((((long)0xf2345678L)) << ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((l) >> (4)) | ((l) << ((sizeof (l) * 8) - (4)))) != (((((long)0xf2345678L)) >> (4)) | ((((long)0xf2345678L)) << ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((ll) >> (shift1)) | ((ll) << ((sizeof (ll) * 8) - (shift1)))) != (((((long long)0xf2345678abcdef0LL)) >> (4)) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) >> (4)) | ((ll) << ((sizeof (ll) * 8) - (4)))) != (((((long long)0xf2345678abcdef0LL)) >> (4)) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) >> (shift2)) | ((ll) << ((sizeof (ll) * 8) - (shift2)))) != (((((long long)0xf2345678abcdef0LL)) >> (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((ll) >> (((sizeof (long long) * 8) - 4))) | ((ll) << ((sizeof (ll) * 8) - (((sizeof (long long) * 8) - 4))))) != (((((long long)0xf2345678abcdef0LL)) >> (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((c) << (shift1)) | ((c) >> ((sizeof (c) * 8) - (shift1)))) != (((((char)0xf234)) << (4)) | ((((char)0xf234)) >> ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((c) << (4)) | ((c) >> ((sizeof (c) * 8) - (4)))) != (((((char)0xf234)) << (4)) | ((((char)0xf234)) >> ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) << (shift1)) | ((s) >> ((sizeof (s) * 8) - (shift1)))) != (((((short)0xf234)) << (4)) | ((((short)0xf234)) >> ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) << (4)) | ((s) >> ((sizeof (s) * 8) - (4)))) != (((((short)0xf234)) << (4)) | ((((short)0xf234)) >> ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) << (shift1)) | ((i) >> ((sizeof (i) * 8) - (shift1)))) != (((((int)0xf234)) << (4)) | ((((int)0xf234)) >> ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) << (4)) | ((i) >> ((sizeof (i) * 8) - (4)))) != (((((int)0xf234)) << (4)) | ((((int)0xf234)) >> ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((l) << (shift1)) | ((l) >> ((sizeof (l) * 8) - (shift1)))) != (((((long)0xf2345678L)) << (4)) | ((((long)0xf2345678L)) >> ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((l) << (4)) | ((l) >> ((sizeof (l) * 8) - (4)))) != (((((long)0xf2345678L)) << (4)) | ((((long)0xf2345678L)) >> ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((ll) << (shift1)) | ((ll) >> ((sizeof (ll) * 8) - (shift1)))) != (((((long long)0xf2345678abcdef0LL)) << (4)) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) << (4)) | ((ll) >> ((sizeof (ll) * 8) - (4)))) != (((((long long)0xf2345678abcdef0LL)) << (4)) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) << (shift2)) | ((ll) >> ((sizeof (ll) * 8) - (shift2)))) != (((((long long)0xf2345678abcdef0LL)) << (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((ll) << (((sizeof (long long) * 8) - 4))) | ((ll) >> ((sizeof (ll) * 8) - (((sizeof (long long) * 8) - 4))))) != (((((long long)0xf2345678abcdef0LL)) << (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  exit (0);
}

