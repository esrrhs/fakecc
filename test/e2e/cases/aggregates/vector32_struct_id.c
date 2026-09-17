// expect: 0
// struct { V32 } is SSE+SSEUP×3 → one YMM, same as a bare 32-byte vector.
package main;
typedef int V __attribute__((vector_size(32)));
struct Box { V x; };
__attribute__((noinline)) struct Box id(struct Box b) { return b; }
int main(void) {
    struct Box a;
    V tmp = { 2, 4, 6, 8, 10, 12, 14, 16 };
    a.x = tmp;
    struct Box r = id(a);
    if (r.x[0] != 2) return 1;
    if (r.x[7] != 16) return 2;
    return 0;
}
