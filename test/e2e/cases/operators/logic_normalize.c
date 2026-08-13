// expect: 7
package main;
int main() {
    /* Result of && must be normalized to 0/1: (3 && 5) == 1. */
    int r = (3 && 5);
    return r + 6;
}
