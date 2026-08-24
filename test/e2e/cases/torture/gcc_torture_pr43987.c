// expect: 0
package main;

typedef unsigned long size_t;
typedef void *FILE;

char B[256 * sizeof(void *)];

typedef struct globals {
    int c;
    FILE *l;
} __attribute__((may_alias)) T;

void add_input_file(FILE *file)
{
  (*(T*)&B).l[0] = file;
}

extern void abort(void);

int main(void)
{
  FILE x;
  (*(T*)&B).l = &x;
  add_input_file ((void *)-1);
  if ((*(T*)&B).l[0] != (void *)-1)
    abort ();
  return 0;
}
