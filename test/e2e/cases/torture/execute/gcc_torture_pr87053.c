// expect: 0
package main;

/* PR middle-end/87053 */

extern void abort (void);
extern unsigned long strlen (const char *);

union U
{ struct {
    char x[4];
    char y[4];
  };
  struct {
    char z[8];
  };
};

const union U u = {{"1234", "567"}};

int main (void)
{
  if (strlen (u.z) != 7)
    abort ();
  return 0;
}
