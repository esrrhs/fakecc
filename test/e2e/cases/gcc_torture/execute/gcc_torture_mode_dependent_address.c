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
typedef signed char my_int8_t;
typedef unsigned char my_uint8_t;
typedef short my_int16_t;
typedef unsigned short my_uint16_t;
typedef int my_int32_t;
typedef unsigned int my_uint32_t;
typedef long long my_int64_t;
typedef unsigned long long my_uint64_t;
typedef long my_intptr_t;
typedef unsigned long my_uintptr_t;
typedef long my_intmax_t;
typedef unsigned long my_uintmax_t;
void f883b (my_int8_t * result,
    my_int16_t *  arg1,
    my_uint32_t *  arg2,
    my_uint64_t *  arg3,
    my_uint8_t *  arg4)
{
    int idx;
    for (idx=0;idx<96;idx += 1) {
 result[idx] = (((((((((((-27 + 2+1)>>1) || arg4[idx]) < arg1[idx])
    ? (((-27 + 2+1)>>1) || arg4[idx])
    : arg1[idx])
          >> (arg2[idx] & 31)) ^ 1) - -32)>>7) | -5) & arg3[idx]);
    }
}
my_int8_t result[96];
my_int16_t arg1[96];
my_uint32_t arg2[96];
my_uint64_t arg3[96];
my_uint8_t arg4[96];
int main(void)
{
  int i;
  int correct[] = {0x0,0x1,0x2,0x3,0x0,0x1,0x2,0x3,0x8,0x9,0xa,0xb,0x8,0x9,
                   0xa,0xb,0x10,0x11,0x12,0x13,0x10,0x11,0x12,0x13,
                   0x18,0x19,0x1a,0x1b,0x18,0x19,0x1a,0x1b,0x20,0x21,0x22,
                   0x23,0x20,0x21,0x22,0x23,0x28,0x29,0x2a,
                   0x2b,0x28,0x29,0x2a,0x2b,0x30,0x31,0x32,0x33,
                   0x30,0x31,0x32,0x33,0x38,0x39,0x3a,0x3b,0x38,0x39,0x3a,
                   0x3b,0x40,0x41,0x42,0x43,0x40,0x41,0x42,0x43,0x48,0x49,
                   0x4a,0x4b,0x48,0x49,0x4a,0x4b,0x50,0x51,
                   0x52,0x53,0x50,0x51,0x52,0x53,0x58,0x59,0x5a,0x5b,
                   0x58,0x59,0x5a,0x5b};
  for (i=0; i < 96; i++)
    arg3[i] = arg2[i] = arg1[i] = arg4[i] = i;
  f883b(result, arg1, arg2, arg3, arg4);
  for (i=0; i < 96; i++)
    if (result[i] != correct[i]) abort();
  return 0;
}

