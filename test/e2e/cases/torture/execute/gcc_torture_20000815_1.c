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

void abort(void);
struct table_elt
{
  void *exp;
  struct table_elt *next_same_hash;
  struct table_elt *prev_same_hash;
  struct table_elt *next_same_value;
  struct table_elt *prev_same_value;
  struct table_elt *first_same_value;
  struct table_elt *related_value;
  int cost;
  int mode;
  char in_memory;
  char in_struct;
  char is_const;
  char flag;
};
struct write_data
{
  int sp : 1;
  int var : 1;
  int nonscalar : 1;
  int all : 1;
};
int cse_rtx_addr_varies_p(void *);
void remove_from_table(struct table_elt *, int);
static struct table_elt *table[32];
void invalidate_memory(struct write_data * writes) {

   int i;
   struct table_elt *p, *next;
  int all = writes->all;
  int nonscalar = writes->nonscalar;
  for (i = 0; i < 31; i++)
    for (p = table[i]; p; p = next)
      {
 next = p->next_same_hash;
 if (p->in_memory
     && (all
  || (nonscalar && p->in_struct)
  || cse_rtx_addr_varies_p (p->exp)))
   remove_from_table (p, i);
      }
}
int cse_rtx_addr_varies_p(void *x) { return 0; }
void remove_from_table(struct table_elt *x, int y) { abort (); }
int main()
{
  struct write_data writes;
  struct table_elt elt;
  memset(&elt, 0, sizeof(elt));
  elt.in_memory = 1;
  table[0] = &elt;
  memset(&writes, 0, sizeof(writes));
  writes.var = 1;
  writes.nonscalar = 1;
  invalidate_memory(&writes);
  return 0;
}

