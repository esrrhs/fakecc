// Int-to-float cast and mixed float/double arithmetic:
//   (float)2 + 2.5 = 4.5 → int 4; plus 1 = 5.
// expect: 5
package main;
int main() {
    int x = 2;
    double y = 2.5;
    return (int)((float)x + y) + 1;
}
