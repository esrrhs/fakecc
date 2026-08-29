package main;
typedef int v2si __attribute__((vector_size(8)));
v2si f(int x) {
  return (v2si) { x, (long)"" };
}
int main() { return 0; }
