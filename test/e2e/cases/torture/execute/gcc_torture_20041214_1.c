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

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_list_tag;
typedef __va_list_tag __builtin_va_list[1];

typedef long unsigned int size_t;
extern void abort (void);
extern char *strcpy (char *, const char *);
extern int strcmp (const char *, const char *);
typedef __builtin_va_list va_list;
static const char null[] = "(null)";
int g (char *s, const char *format, va_list ap)
{
  const char *f;
  const char *string;
  char spec;
  static const void *step0_jumps[] = {
    &&do_precision,
    &&do_form_integer,
    &&do_form_string,
  };
  f = format;
  if (*f == '\0')
    goto all_done;
  do
    {
      spec = (*++f);
      goto *(step0_jumps[2]);
      
    /* begin switch table. */
    do_precision:
      ++f;
      __builtin_va_arg (ap, int);
      spec = *f;
      goto *(step0_jumps[2]);
      
      do_form_integer:
	__builtin_va_arg (ap, unsigned long int);
	goto end;
	
      do_form_string:
	string = __builtin_va_arg (ap, const char *);
	strcpy (s, string);
      
      /* End of switch table. */
      end:
      ++f;
    }
  while (*f != '\0');

all_done:
  return 0;
}

void
f (char *s, const char *f, ...)
{
  va_list ap;
  __builtin_va_start (ap, f);
  g (s, f, ap);
  __builtin_va_end (ap);
}

int
main (void)
{
  char buf[10];
  f (buf, "%s", "asdf", 0);
  if (strcmp (buf, "asdf"))
    abort ();
  return 0;
}
