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
typedef struct _RenderInfo RenderInfo;
struct _RenderInfo
{
    int y;
    float scaley;
    int src_y;
};
static void bar(void) { }
static int
render_image_rgb_a (RenderInfo * info)
{
  int y, ye;
  float error;
  float step;
  y = info->y;
  ye = 256;
  step = 1.0 / info->scaley;
  error = y * step;
  error -= ((int) error) - step;
  for (; y < ye; y++) {
      if (error >= 1.0) {
   info->src_y += (int) error;
   error -= (int) error;
   bar();
      }
      error += step;
  }
  return info->src_y;
}
int main (void)
{
    RenderInfo info;
    info.y = 0;
    info.src_y = 0;
    info.scaley = 1.0;
    if (render_image_rgb_a(&info) != 256)
       abort ();
    exit(0);
}
