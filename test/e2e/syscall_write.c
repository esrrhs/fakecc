// expect: 13
// write(1, "hello world\n", 13) returns 13 on success.
package main;
int main() {
    long r = __syscall(1, 1, "hello world\n", 13);
    return (int)r;
}
