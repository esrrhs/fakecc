package main;
extern int x;
unsigned foo(unsigned int y) {
  return (y << ((long)&x)) | (y >> (32 - ((long)&x)));
}
int main() { return 0; }
