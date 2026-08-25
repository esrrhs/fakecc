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

struct mouse_button_str {
        unsigned char left : 1;
        unsigned char right : 1;
        unsigned char middle : 1;
        } button;
static char fct (struct mouse_button_str newbutton) ;
static char fct(struct mouse_button_str newbutton)
{
  char l = newbutton.left;
  char r = newbutton.right;
  char m = newbutton.middle;
 return l || r || m;
}
int main(void)
{
  struct mouse_button_str newbutton1;
  newbutton1.left = 1;
  newbutton1.middle = 1;
  newbutton1.right = 1;
  if (!fct (newbutton1))
    abort();
  newbutton1.left = 0;
  newbutton1.middle = 1;
  newbutton1.right = 1;
  if (!fct (newbutton1))
    abort();
  newbutton1.left = 1;
  newbutton1.middle = 0;
  newbutton1.right = 1;
  if (!fct (newbutton1))
    abort();
  newbutton1.left = 1;
  newbutton1.middle = 1;
  newbutton1.right = 0;
  if (!fct (newbutton1))
    abort();
  newbutton1.left = 1;
  newbutton1.middle = 0;
  newbutton1.right = 0;
  if (!fct (newbutton1))
    abort();
  newbutton1.left = 0;
  newbutton1.middle = 1;
  newbutton1.right = 0;
  if (!fct (newbutton1))
    abort();
  newbutton1.left = 0;
  newbutton1.middle = 0;
  newbutton1.right = 1;
  if (!fct (newbutton1))
    abort();
  newbutton1.left = 0;
  newbutton1.middle = 0;
  newbutton1.right = 0;
  if (fct (newbutton1))
    abort();
}

