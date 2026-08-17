// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr110166-1.c
package main;

struct s
{
  int t : 1;
  int t1 : 1;
};

int f(struct s t) ;
int f(struct s t)
{
   int c = t.t;
   int d = t.t1;
   if (c > d)
     t.t = d;
   else
     t.t = c;
  return t.t;
}

int main(void)
{
        struct s t;
        for(int i = -1;i <= 0; i++)
        {
          for(int j = -1;j <= 0; j++)
          {
		struct s t = {i, j};
                int r = f(t);
                int exp = i < j ? i : j;
                if (exp != r)
                 return 1;
          }
        }
}