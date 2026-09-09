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

_Bool lt_lt_ne_(int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool lt_lt_ne_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool lt_lt_eq_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool lt_lt_eq_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool lt_lt_xor_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool lt_lt_xor_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool lt_le_ne_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool lt_le_ne_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool lt_le_eq_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool lt_le_eq_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool lt_le_xor_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool lt_le_xor_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool lt_gt_ne_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool lt_gt_ne_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool lt_gt_eq_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool lt_gt_eq_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool lt_gt_xor_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool lt_gt_xor_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool lt_ge_ne_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool lt_ge_ne_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool lt_ge_eq_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool lt_ge_eq_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool lt_ge_xor_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool lt_ge_xor_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool lt_eq_ne_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool lt_eq_ne_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool lt_eq_eq_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool lt_eq_eq_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool lt_eq_xor_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool lt_eq_xor_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool lt_ne_ne_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool lt_ne_ne_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool lt_ne_eq_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool lt_ne_eq_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool lt_ne_xor_ (int a, int b) { _Bool x = ((a) < (b)); _Bool y = ((a) != (b)); return ((x) ^ (y)); } _Bool lt_ne_xor_volatile (int a, int b) { volatile _Bool x = ((a) < (b)); volatile _Bool y = ((a) != (b)); return ((x) ^ (y)); } _Bool le_lt_ne_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool le_lt_ne_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool le_lt_eq_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool le_lt_eq_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool le_lt_xor_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool le_lt_xor_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool le_le_ne_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool le_le_ne_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool le_le_eq_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool le_le_eq_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool le_le_xor_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool le_le_xor_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool le_gt_ne_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool le_gt_ne_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool le_gt_eq_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool le_gt_eq_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool le_gt_xor_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool le_gt_xor_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool le_ge_ne_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool le_ge_ne_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool le_ge_eq_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool le_ge_eq_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool le_ge_xor_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool le_ge_xor_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool le_eq_ne_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool le_eq_ne_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool le_eq_eq_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool le_eq_eq_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool le_eq_xor_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool le_eq_xor_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool le_ne_ne_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool le_ne_ne_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool le_ne_eq_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool le_ne_eq_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool le_ne_xor_ (int a, int b) { _Bool x = ((a) <= (b)); _Bool y = ((a) != (b)); return ((x) ^ (y)); } _Bool le_ne_xor_volatile (int a, int b) { volatile _Bool x = ((a) <= (b)); volatile _Bool y = ((a) != (b)); return ((x) ^ (y)); } _Bool gt_lt_ne_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool gt_lt_ne_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool gt_lt_eq_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool gt_lt_eq_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool gt_lt_xor_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool gt_lt_xor_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool gt_le_ne_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool gt_le_ne_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool gt_le_eq_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool gt_le_eq_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool gt_le_xor_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool gt_le_xor_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool gt_gt_ne_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool gt_gt_ne_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool gt_gt_eq_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool gt_gt_eq_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool gt_gt_xor_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool gt_gt_xor_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool gt_ge_ne_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool gt_ge_ne_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool gt_ge_eq_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool gt_ge_eq_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool gt_ge_xor_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool gt_ge_xor_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool gt_eq_ne_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool gt_eq_ne_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool gt_eq_eq_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool gt_eq_eq_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool gt_eq_xor_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool gt_eq_xor_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool gt_ne_ne_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool gt_ne_ne_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool gt_ne_eq_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool gt_ne_eq_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool gt_ne_xor_ (int a, int b) { _Bool x = ((a) > (b)); _Bool y = ((a) != (b)); return ((x) ^ (y)); } _Bool gt_ne_xor_volatile (int a, int b) { volatile _Bool x = ((a) > (b)); volatile _Bool y = ((a) != (b)); return ((x) ^ (y)); } _Bool ge_lt_ne_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool ge_lt_ne_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) < (b)); return ((x) != (y)); } _Bool ge_lt_eq_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool ge_lt_eq_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) < (b)); return ((x) == (y)); } _Bool ge_lt_xor_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool ge_lt_xor_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) < (b)); return ((x) ^ (y)); } _Bool ge_le_ne_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool ge_le_ne_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) <= (b)); return ((x) != (y)); } _Bool ge_le_eq_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool ge_le_eq_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) <= (b)); return ((x) == (y)); } _Bool ge_le_xor_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool ge_le_xor_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) <= (b)); return ((x) ^ (y)); } _Bool ge_gt_ne_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool ge_gt_ne_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) > (b)); return ((x) != (y)); } _Bool ge_gt_eq_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool ge_gt_eq_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) > (b)); return ((x) == (y)); } _Bool ge_gt_xor_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool ge_gt_xor_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) > (b)); return ((x) ^ (y)); } _Bool ge_ge_ne_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool ge_ge_ne_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) >= (b)); return ((x) != (y)); } _Bool ge_ge_eq_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool ge_ge_eq_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) >= (b)); return ((x) == (y)); } _Bool ge_ge_xor_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool ge_ge_xor_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) >= (b)); return ((x) ^ (y)); } _Bool ge_eq_ne_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool ge_eq_ne_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) == (b)); return ((x) != (y)); } _Bool ge_eq_eq_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool ge_eq_eq_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) == (b)); return ((x) == (y)); } _Bool ge_eq_xor_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool ge_eq_xor_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) == (b)); return ((x) ^ (y)); } _Bool ge_ne_ne_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool ge_ne_ne_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) != (b)); return ((x) != (y)); } _Bool ge_ne_eq_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool ge_ne_eq_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) != (b)); return ((x) == (y)); } _Bool ge_ne_xor_ (int a, int b) { _Bool x = ((a) >= (b)); _Bool y = ((a) != (b)); return ((x) ^ (y)); } _Bool ge_ne_xor_volatile (int a, int b) { volatile _Bool x = ((a) >= (b)); volatile _Bool y = ((a) != (b)); return ((x) ^ (y)); }
int main()
{
  for(int x = -10; x < 10; x++)
    for(int y = -10; y < 10; y++)
    {
        do { if (lt_lt_ne_(x,y) != lt_lt_ne_volatile(x,y)) abort(); } 
while(0); do { if (lt_lt_eq_(x,y) != lt_lt_eq_volatile(x,y)) abort(); } 
while(0); do { if (lt_lt_xor_(x,y) != lt_lt_xor_volatile(x,y)) abort(); } 
while(0); do { if (lt_le_ne_(x,y) != lt_le_ne_volatile(x,y)) abort(); } 
while(0); do { if (lt_le_eq_(x,y) != lt_le_eq_volatile(x,y)) abort(); } 
while(0); do { if (lt_le_xor_(x,y) != lt_le_xor_volatile(x,y)) abort(); } 
while(0); do { if (lt_gt_ne_(x,y) != lt_gt_ne_volatile(x,y)) abort(); } 
while(0); do { if (lt_gt_eq_(x,y) != lt_gt_eq_volatile(x,y)) abort(); } 
while(0); do { if (lt_gt_xor_(x,y) != lt_gt_xor_volatile(x,y)) abort(); } 
while(0); do { if (lt_ge_ne_(x,y) != lt_ge_ne_volatile(x,y)) abort(); } 
while(0); do { if (lt_ge_eq_(x,y) != lt_ge_eq_volatile(x,y)) abort(); } 
while(0); do { if (lt_ge_xor_(x,y) != lt_ge_xor_volatile(x,y)) abort(); } 
while(0); do { if (lt_eq_ne_(x,y) != lt_eq_ne_volatile(x,y)) abort(); } 
while(0); do { if (lt_eq_eq_(x,y) != lt_eq_eq_volatile(x,y)) abort(); } 
while(0); do { if (lt_eq_xor_(x,y) != lt_eq_xor_volatile(x,y)) abort(); } 
while(0); do { if (lt_ne_ne_(x,y) != lt_ne_ne_volatile(x,y)) abort(); } 
while(0); do { if (lt_ne_eq_(x,y) != lt_ne_eq_volatile(x,y)) abort(); } 
while(0); do { if (lt_ne_xor_(x,y) != lt_ne_xor_volatile(x,y)) abort(); } 
while(0); do { if (le_lt_ne_(x,y) != le_lt_ne_volatile(x,y)) abort(); } 
while(0); do { if (le_lt_eq_(x,y) != le_lt_eq_volatile(x,y)) abort(); } 
while(0); do { if (le_lt_xor_(x,y) != le_lt_xor_volatile(x,y)) abort(); } 
while(0); do { if (le_le_ne_(x,y) != le_le_ne_volatile(x,y)) abort(); } 
while(0); do { if (le_le_eq_(x,y) != le_le_eq_volatile(x,y)) abort(); } 
while(0); do { if (le_le_xor_(x,y) != le_le_xor_volatile(x,y)) abort(); } 
while(0); do { if (le_gt_ne_(x,y) != le_gt_ne_volatile(x,y)) abort(); } 
while(0); do { if (le_gt_eq_(x,y) != le_gt_eq_volatile(x,y)) abort(); } 
while(0); do { if (le_gt_xor_(x,y) != le_gt_xor_volatile(x,y)) abort(); } 
while(0); do { if (le_ge_ne_(x,y) != le_ge_ne_volatile(x,y)) abort(); } 
while(0); do { if (le_ge_eq_(x,y) != le_ge_eq_volatile(x,y)) abort(); } 
while(0); do { if (le_ge_xor_(x,y) != le_ge_xor_volatile(x,y)) abort(); } 
while(0); do { if (le_eq_ne_(x,y) != le_eq_ne_volatile(x,y)) abort(); } 
while(0); do { if (le_eq_eq_(x,y) != le_eq_eq_volatile(x,y)) abort(); } 
while(0); do { if (le_eq_xor_(x,y) != le_eq_xor_volatile(x,y)) abort(); } 
while(0); do { if (le_ne_ne_(x,y) != le_ne_ne_volatile(x,y)) abort(); } 
while(0); do { if (le_ne_eq_(x,y) != le_ne_eq_volatile(x,y)) abort(); } 
while(0); do { if (le_ne_xor_(x,y) != le_ne_xor_volatile(x,y)) abort(); } 
while(0); do { if (gt_lt_ne_(x,y) != gt_lt_ne_volatile(x,y)) abort(); } 
while(0); do { if (gt_lt_eq_(x,y) != gt_lt_eq_volatile(x,y)) abort(); } 
while(0); do { if (gt_lt_xor_(x,y) != gt_lt_xor_volatile(x,y)) abort(); } 
while(0); do { if (gt_le_ne_(x,y) != gt_le_ne_volatile(x,y)) abort(); } 
while(0); do { if (gt_le_eq_(x,y) != gt_le_eq_volatile(x,y)) abort(); } 
while(0); do { if (gt_le_xor_(x,y) != gt_le_xor_volatile(x,y)) abort(); } 
while(0); do { if (gt_gt_ne_(x,y) != gt_gt_ne_volatile(x,y)) abort(); } 
while(0); do { if (gt_gt_eq_(x,y) != gt_gt_eq_volatile(x,y)) abort(); } 
while(0); do { if (gt_gt_xor_(x,y) != gt_gt_xor_volatile(x,y)) abort(); } 
while(0); do { if (gt_ge_ne_(x,y) != gt_ge_ne_volatile(x,y)) abort(); } 
while(0); do { if (gt_ge_eq_(x,y) != gt_ge_eq_volatile(x,y)) abort(); } 
while(0); do { if (gt_ge_xor_(x,y) != gt_ge_xor_volatile(x,y)) abort(); } 
while(0); do { if (gt_eq_ne_(x,y) != gt_eq_ne_volatile(x,y)) abort(); } 
while(0); do { if (gt_eq_eq_(x,y) != gt_eq_eq_volatile(x,y)) abort(); } 
while(0); do { if (gt_eq_xor_(x,y) != gt_eq_xor_volatile(x,y)) abort(); } 
while(0); do { if (gt_ne_ne_(x,y) != gt_ne_ne_volatile(x,y)) abort(); } 
while(0); do { if (gt_ne_eq_(x,y) != gt_ne_eq_volatile(x,y)) abort(); } 
while(0); do { if (gt_ne_xor_(x,y) != gt_ne_xor_volatile(x,y)) abort(); } 
while(0); do { if (ge_lt_ne_(x,y) != ge_lt_ne_volatile(x,y)) abort(); } 
while(0); do { if (ge_lt_eq_(x,y) != ge_lt_eq_volatile(x,y)) abort(); } 
while(0); do { if (ge_lt_xor_(x,y) != ge_lt_xor_volatile(x,y)) abort(); } 
while(0); do { if (ge_le_ne_(x,y) != ge_le_ne_volatile(x,y)) abort(); } 
while(0); do { if (ge_le_eq_(x,y) != ge_le_eq_volatile(x,y)) abort(); } 
while(0); do { if (ge_le_xor_(x,y) != ge_le_xor_volatile(x,y)) abort(); } 
while(0); do { if (ge_gt_ne_(x,y) != ge_gt_ne_volatile(x,y)) abort(); } 
while(0); do { if (ge_gt_eq_(x,y) != ge_gt_eq_volatile(x,y)) abort(); } 
while(0); do { if (ge_gt_xor_(x,y) != ge_gt_xor_volatile(x,y)) abort(); } 
while(0); do { if (ge_ge_ne_(x,y) != ge_ge_ne_volatile(x,y)) abort(); } 
while(0); do { if (ge_ge_eq_(x,y) != ge_ge_eq_volatile(x,y)) abort(); } 
while(0); do { if (ge_ge_xor_(x,y) != ge_ge_xor_volatile(x,y)) abort(); } 
while(0); do { if (ge_eq_ne_(x,y) != ge_eq_ne_volatile(x,y)) abort(); } 
while(0); do { if (ge_eq_eq_(x,y) != ge_eq_eq_volatile(x,y)) abort(); } 
while(0); do { if (ge_eq_xor_(x,y) != ge_eq_xor_volatile(x,y)) abort(); } 
while(0); do { if (ge_ne_ne_(x,y) != ge_ne_ne_volatile(x,y)) abort(); } 
while(0); do { if (ge_ne_eq_(x,y) != ge_ne_eq_volatile(x,y)) abort(); } 
while(0); do { if (ge_ne_xor_(x,y) != ge_ne_xor_volatile(x,y)) abort(); } 
while(0);
    }
}

