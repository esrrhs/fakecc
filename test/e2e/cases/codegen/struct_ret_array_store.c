// expect: 220
// 16-byte INTEGER-class struct returns stored into an array and a named
// local must keep the rax/rdx payload (no dummy keep-alloca liveness hack).
//  t.c + sum_i (arr[i].c + arr[i].a) = 20 + (20+40+60+80) = 220.
package main;

typedef struct { int a; int b; long long c; } S;

static S mk(long long x) {
    S s;
    s.a = (int)x;
    s.b = (int)x + 1;
    s.c = x;
    return s;
}

int main(void) {
    S arr[4];
    S t;
    int i;
    long long sum;

    arr[0] = mk(10);
    arr[1] = mk(20);
    arr[2] = mk(30);
    arr[3] = mk(40);
    t = mk(20);
    sum = t.c;
    for (i = 0; i < 4; i++)
        sum += arr[i].c + arr[i].a;
    return (int)sum;
}
