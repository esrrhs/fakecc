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
typedef unsigned int my_dev_t;
typedef unsigned int kdev_t;
static  kdev_t to_kdev_t(int dev)
{
 int major, minor;
 if (sizeof(kdev_t) == 16)
  return (kdev_t)dev;
 major = (dev >> 8);
 minor = (dev & 0xff);
 return ((( major ) << 22 ) | ( minor )) ;
}
void do_mknod(const char * filename, int mode, kdev_t dev)
{
 if (dev==0x15800078)
  exit(0);
 else
  abort();
}
char * getname(const char * filename)
{
  unsigned int a1,a2,a3,a4,a5,a6,a7,a8,a9;
 a1 = (unsigned int)(filename) *5 + 1;
 a2 = (unsigned int)(filename) *6 + 2;
 a3 = (unsigned int)(filename) *7 + 3;
 a4 = (unsigned int)(filename) *8 + 4;
 a5 = (unsigned int)(filename) *9 + 5;
 a6 = (unsigned int)(filename) *10 + 5;
 a7 = (unsigned int)(filename) *11 + 5;
 a8 = (unsigned int)(filename) *12 + 5;
 a9 = (unsigned int)(filename) *13 + 5;
 return (char *)(a1*a2+a3*a4+a5*a6+a7*a8+a9);
}
int sys_mknod(const char * filename, int mode, my_dev_t dev)
{
 int error;
 char * tmp;
 tmp = getname(filename);
 error = ((long)( tmp )) ;
 do_mknod(tmp,mode,to_kdev_t(dev));
 return error;
}
int main(void)
{
 if (sizeof (int) != 4)
   exit (0);
 return sys_mknod("test",1,0x12345678);
}

