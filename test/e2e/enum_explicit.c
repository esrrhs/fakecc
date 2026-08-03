// expect: 5
package main;
enum Flag { A = 1, B = 2, C = 4, D = 8 };
int main() {
    /* Explicit enum values; C | A == 4 | 1 == 5. */
    return C | A;
}
