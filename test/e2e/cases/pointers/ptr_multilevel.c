// expect: 77
package main;
int main() {
    int x = 77;
    int *p = &x;
    int **pp = &p;
    return **pp;
}
