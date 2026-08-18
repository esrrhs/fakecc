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
extern void exit (int);
extern int printf (const char *,...);
struct s { struct s *n; } *p;
struct s ss;
struct s sss[10];
int count = 0;
void sub( struct s *p, struct s **pp );
int look( struct s *p, struct s **pp );
int main(void)
{
    struct s *pp;
    struct s *next;
    int i;
    p = &ss;
    next = p;
    for ( i = 0; i < 10; i++ ) {
        next->n = &sss[i];
        next = next->n;
    }
    next->n = 0;
    sub( p, &pp );
    if (count != 10 +2)
      abort ();
    exit( 0 );
}
void sub(struct s *p, struct s **pp)
{
   for ( ; look( p, pp ); ) {
        if ( p )
            p = p->n;
        else
            break;
   }
}
int look(struct s *p, struct s **pp)
{
    for ( ; p; p = p->n )
        ;
    *pp = p;
    count++;
    return( 1 );
}

