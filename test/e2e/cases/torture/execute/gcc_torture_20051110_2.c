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

void add_unwind_adjustsp (long);
void abort (void);
unsigned char bytes[5];
int flag;
void
add_unwind_adjustsp (long offset)
{
  int n;
  unsigned long o;
  o = (long) ((offset - 0x204) >> 2);
  n = 0;
  do
    {
a:
      bytes[n] = o & 0x7f;
      o >>= 7;
      if (o)
        {
   bytes[n] |= 0x80;
   if (flag)
     goto a;
 }
      n++;
    }
  while (o);
}
int main(void)
{
  add_unwind_adjustsp (4132);
  if (bytes[0] != 0x88 || bytes[1] != 0x07)
    abort ();
  return 0;
}
