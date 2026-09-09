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
struct vc_data {
 unsigned long space;
 unsigned char vc_palette[16*3];
};
struct vc {
 struct vc_data *d;
};
struct vc_data a_con;
struct vc vc_cons[63] = { &a_con };
int default_red[16];
int default_grn[16];
int default_blu[16];
extern void bar(int);
void reset_palette(int currcons)
{
 int j, k;
 for (j=k=0; j<16; j++) {
  (vc_cons[currcons].d->vc_palette) [k++] = default_red[j];
  (vc_cons[currcons].d->vc_palette) [k++] = default_grn[j];
  (vc_cons[currcons].d->vc_palette) [k++] = default_blu[j];
 }
 
bar(k);
}
void bar(int k)
{
 if (k != 16*3)
  abort();
}
int main()
{
 reset_palette(0);
 exit(0);
}

