// expect: 5
// runtime.strlen over a string literal, returned as the exit code.
// skip_oracle: clang
package main;
import runtime;
int main(void) { return (int)runtime.strlen("hello"); }
