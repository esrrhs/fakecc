// expect: 0
// Block-scope `&(int){42}` must materialize storage and yield its address,
// not treat the loaded scalar 42 as a pointer.
package main;
int main(void) {
    int *p = &(int){42};
    if (*p != 42) return 1;
    return 0;
}
