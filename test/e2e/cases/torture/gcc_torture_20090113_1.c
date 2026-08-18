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

typedef struct descriptor_dimension
{
  int stride;
  int lbound;
  int ubound;
} descriptor_dimension;
typedef struct {
    int *data;
    int dtype;
    descriptor_dimension dim[7];
} gfc_array_i4;
void
msum_i4 (gfc_array_i4 * const retarray,
  gfc_array_i4 * const array,
  const int * const pdim)
{
  int count[7];
  int extent[7];
  int * dest;
  const int * base;
  int dim;
  int n;
  int len;
  dim = (*pdim) - 1;
  len = array->dim[dim].ubound + 1 - array->dim[dim].lbound;
  for (n = 0; n < dim; n++)
    {
      extent[n] = array->dim[n].ubound + 1 - array->dim[n].lbound;
      count[n] = 0;
    }
  dest = retarray->data;
  base = array->data;
  do
    {
      int result = 0;
      for (n = 0; n < len; n++, base++)
 result += *base;
      *dest = result;
      count[0]++;
      dest += 1;
    }
  while (count[0] != extent[0]);
}
int main()
{
  int rdata[3];
  int adata[9];
  gfc_array_i4 retarray = { rdata, 265, { { 1, 1, 3 } } };
  gfc_array_i4 array = { adata, 266, { { 1, 1, 3 }, { 3, 1, 3 } } };
  int dim = 2;
  msum_i4 (&retarray, &array, &dim);
  return 0;
}
