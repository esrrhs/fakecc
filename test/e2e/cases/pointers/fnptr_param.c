// expect: 7
// Function-pointer parameter: a function taking an int(*)(int,int) param,
// passing a function lvalue, and calling indirectly through the param.
package main;
int add(int a, int b) { return a + b; }
int apply(int x, int y, int (*op)(int, int)) { return op(x, y); }
int main(void) {
    return apply(3, 4, add);
}
