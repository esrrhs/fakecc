// expect: 0
package main;

extern void abort(void);
void abort(void);

static void
foo (const char *x)
{

  char a = '0';
  while (1)
    {
      switch (*x)
	{
	case '_':
	case '+':
	  a = *x;
	  x++;
	  continue;
	default:
	  break;
	}
      break;
    }
  if (a == '0' || a == '+')
    abort ();
}

int
main ()
{
  foo ("_");
  return 0;
}
