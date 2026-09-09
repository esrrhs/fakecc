package main;
typedef int v2si __attribute__((vector_size(8)));
v2si f(int x) {
  return (v2si) { (long)"", x };
}
int main() { return 0; }
