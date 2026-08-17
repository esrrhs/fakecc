// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr110165-1.c
package main;

struct s
{
  int t : 1;
};

int f(struct s t, int a, int b) ;
int f(struct s t, int a, int b)
{
        int bd = t.t;
        if (bd) a|=b;
        return a;
}

int main(void)
{
        struct s t;
        for(int i = -1;i <= 1; i++)
        {
                int a = 0x10;
                int b = 0x0f;
                int c = a | b;
		struct s t = {i};
                int r = f(t, a, b);
                int exp = (i != 0) ? a | b : a;
                if (exp != r)
                 return 1;
        }
}