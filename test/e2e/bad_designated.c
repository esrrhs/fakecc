// expect_error
// An array designator with an out-of-range index (`[5]` for an `int[3]`) must be
// rejected at compile time.  fakecc must report a designator-range error.
package main;
int main() {
    int a[3] = {[5] = 1};
    return a[0];
}
