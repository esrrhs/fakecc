// expect: 0
package main;

static int g;

__attribute__((destructor(200))) void dtor_late(void) { g = 1; }
__attribute__((destructor(100))) void dtor_early(void) { g = 2; }

int main(void) {
    return 0;
}
