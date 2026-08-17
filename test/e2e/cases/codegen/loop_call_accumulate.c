// expect: 21
// A loop containing a function call: the accumulator is initialized before
// the loop, modified inside the loop body across a call, and read after.
// Exercises φ-nodes at the loop header whose incoming values cross a call
// boundary — a pattern that regalloc must handle correctly under -g.
// Markers present under -g must not change the generated code.
package main;
int add(int a, int b) {
    return a + b;
}
int main(void) {
    int sum = 0;
    int i;
    for (i = 1; i <= 6; i = i + 1) {
        sum = add(sum, i);
    }
    return sum;
}
