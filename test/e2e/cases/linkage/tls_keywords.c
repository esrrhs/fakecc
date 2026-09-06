// expect: 30
package main;

static __thread int x;
_Thread_local int y;
thread_local int z;

int main() {
    x = 5;
    y = 10;
    z = 15;
    return x + y + z;
}
