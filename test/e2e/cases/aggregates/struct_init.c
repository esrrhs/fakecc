// expect: 9
// Struct initializer list: define the struct at file scope, then init a
// local variable positionally.
package main;
struct S { int a; int b; };
int main() {
    struct S s = {4, 5};
    return s.a + s.b;
}
