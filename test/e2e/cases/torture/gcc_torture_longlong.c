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
typedef unsigned long long uint64;
unsigned long pars;
uint64 b[32];
uint64 *r = b;
void alpha_ep_extbl_i_eq_0()
{
  unsigned int rb, ra, rc;
  rb = (((unsigned long)(pars) >> 27)) & 0x1fUL;
  ra = (((unsigned int)(pars) >> 5)) & 0x1fUL;
  rc = (((unsigned int)(pars) >> 0)) & 0x1fUL;
  {
    uint64 temp = ((r[ra] >> ((r[rb] & 0x7) << 3)) & 0x00000000000000FFLL);
    if (rc != 31)
      r[rc] = temp;
  }
}
int
main(void)
{
  if (sizeof (uint64) == 8)
    {
      b[17] = 0x0000000000303882ULL;
      b[2] = 0x534f4f4c494d000aULL;
      pars = 0x88000042;
      alpha_ep_extbl_i_eq_0();
      if (b[2] != 0x4d)
 abort ();
    }
  exit (0);
}
