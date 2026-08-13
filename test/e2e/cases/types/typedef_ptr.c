// expect: 17
// Typedef for a pointer type: `typedef int* Pint`.
package main;
typedef int* Pint;
int main() {
    int x = 17;
    Pint p = &x;
    return *p;
}
