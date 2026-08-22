// GCC simple escapes: \a \b \f \v (and \e as 27) must not fall through as
// the letter itself.  '\f' is form feed 12, not 'f'.
// expect: 0
package main;
int main() {
    if ('\a' != 7) return 1;
    if ('\b' != 8) return 2;
    if ('\f' != 12) return 3;
    if ('\v' != 11) return 4;
    if ('\e' != 27) return 5;
    if ("\f"[0] != 12) return 6;
    if ("\v"[0] != 11) return 7;
    return 0;
}
