// expect: 5
// Use extern libc strlen over a string literal, returned as the exit code.
package main;
extern long strlen(const char *s);
int main(void) { return (int)strlen("hello"); }
