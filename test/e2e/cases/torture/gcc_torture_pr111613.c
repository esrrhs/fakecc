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
struct bitfield {
 unsigned int field1 : 1;
 unsigned int field2 : 1;
 unsigned int field3 : 1;
};
 static void set_field1_and_field2(struct bitfield *b) {
 b->field1 = 1;
 b->field2 = 1;
}

 static struct bitfield * new_bitfield(void) {
 struct bitfield *b = (struct bitfield *)malloc(sizeof(*b));
 b->field3 = 1;
 set_field1_and_field2(b);
 return b;
}
int main(void) {
 struct bitfield *b = new_bitfield();
 if (b->field3 != 1)
  abort();
 return 0;
}

