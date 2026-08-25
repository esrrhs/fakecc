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

typedef signed short SInt16;
typedef struct {
    SInt16 minx;
    SInt16 maxx;
    SInt16 miny;
    SInt16 maxy;
} IOGBounds;
int expectedwidth = 50;
unsigned int *global_vramPtr = (unsigned int *)0xa000;
IOGBounds global_bounds = { 100, 150, 100, 150 };
IOGBounds global_saveRect = { 75, 175, 75, 175 };
int
main(void)
{
  unsigned int *vramPtr;
  int width;
  IOGBounds saveRect = global_saveRect;
  IOGBounds bounds = global_bounds;
  if (saveRect.minx < bounds.minx) saveRect.minx = bounds.minx;
  if (saveRect.maxx > bounds.maxx) saveRect.maxx = bounds.maxx;
  vramPtr = global_vramPtr + (saveRect.miny - bounds.miny) ;
  width = saveRect.maxx - saveRect.minx;
  if (width != expectedwidth)
    abort ();
  exit (0);
}
