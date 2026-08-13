// expect: 12
// expect_stdout: hello world
// write(1, "hello world\n", 12) returns 12 on success and actually writes.
package main;
int main() {
    long r = __syscall(1, 1, "hello world\n", 12);
    return (int)r;
}
