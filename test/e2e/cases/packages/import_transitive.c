// expect: 0
/* Transitive import: fmt pulls in io; we only import fmt but call through it. */
package main;
import fmt;
int main(void) {
    fmt.printf("ok\n");
    return 0;
}
