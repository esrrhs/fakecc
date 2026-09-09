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

struct rtattr
{
  unsigned short rta_len;
  unsigned short rta_type;
};
__attribute__ ((noinline))
int inet_check_attr (void *r, struct rtattr **rta)
{
  int i;
  for (i = 1; i <= 14; i++)
    {
      struct rtattr *attr = rta[i - 1];
      if (attr)
 {
   if (attr->rta_len - sizeof (struct rtattr) < 4)
     return -22;
   if (i != 9 && i != 8)
     rta[i - 1] = attr + 1;
 }
    }
  return 0;
}
extern void abort (void);
int
main (void)
{
  struct rtattr rt[2];
  struct rtattr *rta[14];
  int i;
  rt[0].rta_len = sizeof (struct rtattr) + 8;
  rt[0].rta_type = 0;
  rt[1] = rt[0];
  for (i = 0; i < 14; i++)
    rta[i] = &rt[0];
  if (inet_check_attr (0, rta) != 0)
    abort ();
  for (i = 0; i < 14; i++)
    if (rta[i] != &rt[i != 7 && i != 8])
      abort ();
  for (i = 0; i < 14; i++)
    rta[i] = &rt[0];
  rta[1] = 0;
  rt[1].rta_len -= 8;
  rta[5] = &rt[1];
  if (inet_check_attr (0, rta) != -22)
    abort ();
  for (i = 0; i < 14; i++)
    if (i == 1 && rta[i] != 0)
      abort ();
    else if (i != 1 && i <= 5 && rta[i] != &rt[1])
      abort ();
    else if (i > 5 && rta[i] != &rt[0])
      abort ();
  return 0;
}
