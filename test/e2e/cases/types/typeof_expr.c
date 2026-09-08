// expect: 0
package main;

struct Point {
    int x;
    double y;
};

int add(int a, int b) {
    return a + b;
}

int main() {
    // 1. typeof from local variable
    int a = 10;
    typeof(a) b = 20;
    if (sizeof(b) != sizeof(int) || b != 20) return 1;

    // 2. typeof from array indexing
    double arr[4];
    arr[0] = 1.5;
    arr[1] = 2.5;
    arr[2] = 3.5;
    arr[3] = 4.5;
    typeof(arr[1]) val = arr[2];
    if (sizeof(val) != sizeof(double) || val != 3.5) return 2;

    // 3. typeof from struct member & pointer dereference
    struct Point pt;
    pt.x = 42;
    pt.y = 3.14;
    struct Point *ptr = &pt;
    typeof(ptr->x) px = pt.x;
    typeof(ptr->y) py = ptr->y;
    if (sizeof(px) != sizeof(int) || px != 42) return 3;
    if (sizeof(py) != sizeof(double) || py != 3.14) return 4;

    // 4. typeof from binary arithmetic expression
    int i = 5;
    double d = 2.5;
    typeof(i + d) mixed = i + d;
    if (sizeof(mixed) != sizeof(double) || mixed != 7.5) return 5;

    // 5. typeof from function call
    typeof(add(1, 2)) sum = add(a, b);
    if (sizeof(sum) != sizeof(int) || sum != 30) return 6;

    // 6. typeof with statement expression and _Complex
    __complex__ double c1;
    __real__ c1 = 3.0;
    __imag__ c1 = 4.0;
    __complex__ double c_arr[2];
    c_arr[0] = c1;
    c_arr[1] = -c1;
    double imag_part = ({
        typeof(c_arr[0]) item = c_arr[0];
        __imag__ item;
    });
    if (imag_part != 4.0) return 7;

    // 7. Scoped block
    {
        double sub_var = 99.5;
        typeof(sub_var) shadow = sub_var + 0.5;
        if (sizeof(shadow) != sizeof(double) || shadow != 100.0) return 8;
    }
    typeof(a) outer = a + 5;
    if (sizeof(outer) != sizeof(int) || outer != 15) return 9;

    return 0;
}
