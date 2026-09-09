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

extern void abort(void);
struct epic_rx_desc
{
  unsigned int next;
};
struct epic_private
{
  struct epic_rx_desc *rx_ring;
  unsigned int rx_skbuff[5];
};
static void epic_init_ring(struct epic_private *ep)
{
  int i;
  for (i = 0; i < 5; i++)
  {
    ep->rx_ring[i].next = 10 + (i+1)*2;
    ep->rx_skbuff[i] = 0;
  }
  ep->rx_ring[i-1].next = 10;
}
static int check_rx_ring[5] = { 12,14,16,18,10 };
int main(void)
{
  struct epic_private ep;
  struct epic_rx_desc rx_ring[5];
  int i;
  for (i=0;i<5;i++)
  {
    rx_ring[i].next=0;
    ep.rx_skbuff[i]=5;
  }
  ep.rx_ring=rx_ring;
  epic_init_ring(&ep);
  for (i=0;i<5;i++)
  {
    if ( rx_ring[i].next != check_rx_ring[i] ) abort();
    if ( ep.rx_skbuff[i] != 0 ) abort();
  }
  return 0;
}
