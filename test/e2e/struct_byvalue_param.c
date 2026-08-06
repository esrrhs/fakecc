// expect: 7
// Struct by value as a parameter: a function taking an int,int,int(*)(int,int)
// is already covered by fnptr_param.  Here we pass a small struct BY VALUE and
// the callee reads its members — the defining test for struct-by-value params.
package main;
struct S { int x; int y; };
int sum(struct S s) { return s.x + s.y; }
int main(void) {
    struct S p;
    p.x = 3; p.y = 4;
    return sum(p);
}
