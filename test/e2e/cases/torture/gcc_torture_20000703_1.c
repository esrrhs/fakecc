// expect: 0
package main;

extern void abort(void);
extern void exit(int);

struct baz {
    int a, b, c, d, e;
};

void foo(struct baz *p, const char *x) {
    p->a = 1;
    p->b = 2;
    p->c = 3;
    p->d = 4;
    p->e = 5;
}

void bar(struct baz *p) {
    if (p->a != 1 || p->b != 2 || p->c != 3 || p->d != 4 || p->e != 5)
        abort();
}

int main() {
    struct baz p;
    foo(&p, ({
        const char *s = "hello";
        s;
    }));
    bar(&p);
    return 0;
}
