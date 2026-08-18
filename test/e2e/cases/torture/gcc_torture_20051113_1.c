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

extern void abort(void);
extern void *malloc(long unsigned int);
extern void *memset(void *, int, long unsigned int);
typedef struct
{
  short a;
  unsigned short b;
  unsigned short c;
  unsigned long long Count;
  long long Count2;
} 
 Struct1;
typedef struct
{
  short a;
  unsigned short b;
  unsigned short c;
  unsigned long long d;
  long long e;
  long long f;
} 
 Struct2;
typedef union
{
  Struct1 a;
  Struct2 b;
} Union;
typedef struct
{
  int Count;
  Union List[0];
} 
 Struct3;
unsigned long long Sum (Struct3 *instrs) ;
unsigned long long Sum(Struct3 *instrs)
{
    unsigned long long count = 0;
    int i;
    for (i = 0; i < instrs->Count; i++) {
        count += instrs->List[i].a.Count;
    }
    return count;
}
long long Sum2 (Struct3 *instrs) ;
long long Sum2(Struct3 *instrs)
{
    long long count = 0;
    int i;
    for (i = 0; i < instrs->Count; i++) {
        count += instrs->List[i].a.Count2;
    }
    return count;
}
int main(void) {
  Struct3 *p = malloc (sizeof (int) + 3 * sizeof(Union));
  memset(p, 0, sizeof(int) + 3*sizeof(Union));
  p->Count = 3;
  p->List[0].a.Count = 555;
  p->List[1].a.Count = 999;
  p->List[2].a.Count = 0x101010101ULL;
  p->List[0].a.Count2 = 555;
  p->List[1].a.Count2 = 999;
  p->List[2].a.Count2 = 0x101010101LL;
  if (Sum(p) != 555 + 999 + 0x101010101ULL)
    abort();
  if (Sum2(p) != 555 + 999 + 0x101010101LL)
    abort();
  return 0;
}

