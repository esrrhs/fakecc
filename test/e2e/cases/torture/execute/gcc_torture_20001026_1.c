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

extern void abort (void);
typedef struct {
  long r[(19 + sizeof (long))/(sizeof (long))];
} realvaluetype;
typedef void *tree;
static realvaluetype real_value_from_int_cst(tree x, tree y)
{
  realvaluetype r;
  int i;
  for (i = 0; i < sizeof(r.r)/sizeof(long); ++i)
    r.r[i] = -1;
  return r;
}
struct brfic_args
{
  tree type;
  tree i;
  realvaluetype d;
};
static void build_real_from_int_cst_1(void * data) {

  struct brfic_args *args = (struct brfic_args *) data;
  args->d = real_value_from_int_cst (args->type, args->i);
}
int main()
{
  struct brfic_args args;
  memset (&args, 0, sizeof(args));
  build_real_from_int_cst_1 (&args);
  if (args.d.r[0] == 0)
    abort ();
  return 0;
}

