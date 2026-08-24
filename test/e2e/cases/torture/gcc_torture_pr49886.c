// expect: 0
package main;

struct PMC {
    unsigned flags;
};

typedef struct Pcc_cell
{
    struct PMC *p;
    long bla;
    long type;
} Pcc_cell;

int gi;
int cond;

extern void abort(void);

void never_ever(int interp, struct PMC *pmc)
  __attribute__((noinline));

void never_ever(int interp, struct PMC *pmc)
{
  abort();
}

static void mark_cell(int * interp, Pcc_cell *c)
  __attribute__((__nonnull__(1)));

static void
mark_cell(int * interp, Pcc_cell *c)
{
  if (!cond)
    return;

  if (c && c->type == 4 && c->p
      && !(c->p->flags & (1<<18)))
    never_ever(gi + 1, c->p);
  if (c && c->type == 4 && c->p
      && !(c->p->flags & (1<<17)))
    never_ever(gi + 2, c->p);
}

static struct Pcc_cell *
__attribute__((noinline))
getnull(void)
{
  return (struct Pcc_cell *) 0;
}

int main(void)
{
  int i;

  cond = 1;
  for (i = 0; i < 100; i++)
    mark_cell (&gi, getnull ());
  return 0;
}
