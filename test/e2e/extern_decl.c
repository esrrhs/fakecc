// expect: 0
// extern declaration at file scope — declaration only, no use.  Parses and
// emits without allocating storage.
package main;
extern int something;
int main() {
    return 0;
}
