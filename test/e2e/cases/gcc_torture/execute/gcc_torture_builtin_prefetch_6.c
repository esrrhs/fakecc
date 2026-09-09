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
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
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
extern int isprint(int);
extern void *stdin;
extern void *stdout;
extern void *stderr;
extern int open(const char*, int, ...);
extern int close(int);
extern long read(int, void*, unsigned long);
extern int unlink(const char*);
extern char* tmpnam(char*);

void exit (int);
int *bad_addr[65];
int arr_used;
void init_addrs(void)
{
  int i;
  int bits_per_ptr = sizeof (void *) * 8;
  for (i = 0; i < bits_per_ptr; i++)
    bad_addr[i] = (void *)(1UL << i);
  arr_used = bits_per_ptr + 1;
}
void prefetch_for_read(void)
{
  int i;
  for (i = 0; i < 65; i++)
    __builtin_prefetch (bad_addr[i], 0, 0);
}
void prefetch_for_write(void)
{
  int i;
  for (i = 0; i < 65; i++)
    __builtin_prefetch (bad_addr[i], 1, 0);
}
int main()
{
  init_addrs ();
  prefetch_for_read ();
  prefetch_for_write ();
  exit (0);
}
