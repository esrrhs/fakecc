// expect: 25
package main;
int main() {
    /* continue inside do-while jumps to condition check.
     * Sum of even numbers 0,2,4,6,8,10 -> wait: only evens 0..10 = 30.
     * Here: sum of 1,3,5,7,9 = 25 (odd numbers, via continue). */
    int s = 0;
    int i = 0;
    do {
        i = i + 1;
        if (i % 2 == 0) { continue; }
        s = s + i;
    } while (i < 10);
    return s;
}
