// Float negation: -(1.5 + 1.5) = -3.0 -> int -3.  SysV returns int via EAX,
// so -3 is returned as a negative exit code (253 unsigned).  Test the
// magnitude via +3 offset: -3 + 3 = 0... instead just check the raw bits by
// computing -((int)(1.5 + 1.5)) which is -3, returned as exit code 253.
// Use a helper to observe the negated float as an int.
// expect: 253
package main;
int main() {
    float a = 1.5;
    float b = 1.5;
    int s = (int)(a + b);   // 3
    float n = -(a + b);     // -3.0
    return (int)n;          // -3 -> exit 253
}
