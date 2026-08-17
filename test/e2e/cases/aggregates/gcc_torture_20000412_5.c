// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000412-5.c
package main;

int main( void ) {
    struct {
	int node;
	int type;
    } lastglob[1] = { { 0   , 1  } };

    if (lastglob[0].node != 0 || lastglob[0].type != 1)
      return 1;
    return 0;
}