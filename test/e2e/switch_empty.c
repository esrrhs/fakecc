// expect: 7
// An empty switch is a no-op.
package main;
int main(void) {
    int x = 1;
    switch (x) {}
    return 7;
}
