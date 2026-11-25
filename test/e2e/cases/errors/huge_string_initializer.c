// expect_error
// A string initializer of a huge array skips the dense slot array and
// used to calloc the whole object in IR.  Reject it instead.
package main;
char buf[2000000000] = "x";
int main(void) { return buf[0]; }
