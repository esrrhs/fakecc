// expect: 5
// Typedef for an array element type, then use it to declare an array.
package main;
typedef int Num;
int main() {
    Num a[3];
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[2] = 5; /* overwrite */
    return a[2];
}
