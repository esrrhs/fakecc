// expect: 5
package main;
int main() {
    int i = 5;
    int r = (i++);
    /* Postfix ++ returns the old value (5), then i becomes 6. */
    return r;
}
