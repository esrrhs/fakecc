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

typedef unsigned long size_t;

extern void abort (void);

int a;

static void __attribute__ ((noinline,noclone))
do_something (int item)
{
  a = item;
}

int
pack_unpack (char *s, char *p)
{
  char *send, *pend;
  char type;
  int integer_size;

  send = s + strlen (s);
  pend = p + strlen (p);

  while (p < pend)
    {
      type = *p++;

      switch (type)
	{
	case 's':
	  integer_size = 2;
	  goto unpack_integer;

	case 'l':
	  integer_size = 4;
	  goto unpack_integer;

	unpack_integer:
	  switch (integer_size)
	    {
	    case 2:
	      {
		union
		{
		  short i;
		  char a[sizeof (short)];
		}
		v;
		memcpy (v.a, s, sizeof (short));
		s += sizeof (short);
		do_something (v.i);
	      }
	      break;

	    case 4:
	      {
		union
		{
		  int i;
		  char a[sizeof (int)];
		}
		v;
		memcpy (v.a, s, sizeof (int));
		s += sizeof (int);
		do_something (v.i);
	      }
	      break;
	    }
	  break;
	}
    }
  return (int) *s;
}

int
main (void)
{
  int n = pack_unpack ("\200\001\377\376\035\300", "sl");
  if (n != 0)
    abort ();
  return 0;
}
