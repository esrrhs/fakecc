// expect: 0
package main;

extern void abort(void);
extern void exit(int);

int g(int a, int b, int (*gi)(int, int)) {
    if ((*gi)(a, b))
        return a;
    else
        return b;
}

void f() {
    int i, j;
    int f2(int a, int b) {
        return a > b;
    }

    if (g(1, 2, f2) != 2)
        abort();
}

int main(void) {
    f();
    return 0;
}
