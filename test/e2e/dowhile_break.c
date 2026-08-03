// expect: 3
package main;
int main() {
    /* break inside do-while exits the loop. */
    int x = 0;
    do {
        x = x + 1;
        if (x == 3) { break; }
    } while (1);
    return x;
}
