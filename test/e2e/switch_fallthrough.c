// expect: 11
package main;
int main() {
    /* Fall-through: case 1 matches, runs case 1 then falls to case 2. */
    int x = 1;
    int r = 0;
    switch (x) {
        case 0: r = 10; break;
        case 1: r = r + 1;         /* no break — falls through to case 2 */
        case 2: r = r + 10; break; /* r = 1 + 10 = 11 */
    }
    return r;
}
