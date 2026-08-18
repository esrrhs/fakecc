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

int strcmp (const char *, const char *);
int ap_standalone;
const char * ap_check_cmd_context(void *a, int b)
{
  return 0;
}
const char * server_type(void *a, void *b, char *arg)
{
  const char *err = ap_check_cmd_context (a, 0x01|0x02|0x04|0x08|0x10);
  if (err)
    return err;
  if (!strcmp (arg, "inetd"))
    ap_standalone = 0;
  else if (!strcmp (arg, "standalone"))
      ap_standalone = 1;
  else
    return "ServerType must be either 'inetd' or 'standalone'";
  return 0;
}
int main()
{
  server_type (0, 0, "standalone");
  return 0;
}

