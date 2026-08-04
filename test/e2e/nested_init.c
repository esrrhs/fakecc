// expect: 21
// Nested initializer: int a[2][3] = {{1,2,3},{4,5,6}}; sum everything.
package main;
int main() {
    int a[2][3] = {{1, 2, 3}, {4, 5, 6}};
    return a[0][0] + a[0][1] + a[0][2] + a[1][0] + a[1][1] + a[1][2];
}
