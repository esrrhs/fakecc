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
typedef unsigned long HARD_REG_SET[2];
HARD_REG_SET reg_class_contents[2];
struct du_chain
{
  struct du_chain *next_use;
  int cl;
};
void __attribute__((noinline))
merge_overlapping_regs (HARD_REG_SET *p)
{
  if ((*p)[0] != -1 || (*p)[1] != -1)
    abort ();
}
void __attribute__((noinline))
regrename_optimize (struct du_chain *this)
{
  HARD_REG_SET this_unavailable;
  unsigned long *scan_fp_;
  int n_uses;
  struct du_chain *last;
  this_unavailable[0] = 0;
  this_unavailable[1] = 0;
  n_uses = 0;
  for (last = this; last->next_use; last = last->next_use)
    {
      scan_fp_ = reg_class_contents[last->cl];
      n_uses++;
      this_unavailable[0] |= ~ scan_fp_[0];
      this_unavailable[1] |= ~ scan_fp_[1];
    }
  if (n_uses < 1)
    return;
  scan_fp_ = reg_class_contents[last->cl];
  this_unavailable[0] |= ~ scan_fp_[0];
  this_unavailable[1] |= ~ scan_fp_[1];
  merge_overlapping_regs (&this_unavailable);
}
int main()
{
  struct du_chain du1 = { 0, 0 };
  struct du_chain du0 = { &du1, 1 };
  reg_class_contents[0][0] = -1;
  reg_class_contents[0][1] = -1;
  reg_class_contents[1][0] = 0;
  reg_class_contents[1][1] = 0;
  regrename_optimize (&du0);
  return 0;
}
