// str.strncmp: like str.strcmp but stops after n bytes.  Must still stop at
// \0 within the window, return 0 when the first n bytes match even
// if the strings differ later, and treat n==0 as always equal.
// expect: 0
package main;
import str;
int main() {
    /* first 3 bytes equal, differ only later -> 0 */
    if (str.strncmp("abcdef", "abcxyz", 3) != 0) return 1;
    /* differ within the window */
    if (str.strncmp("abc", "abd", 3) >= 0) return 2;
    if (str.strncmp("abd", "abc", 3) <= 0) return 3;
    /* n == 0 always equal */
    if (str.strncmp("abc", "xyz", 0) != 0) return 4;
    /* stop at \0 within window: both strings terminate at the same
       position inside the window -> equal via the \0 check */
    if (str.strncmp("ab", "ab", 3) != 0) return 5;
    /* window shorter than the difference -> equal */
    if (str.strncmp("abcdef", "abcxyz", 2) != 0) return 6;
    return 0;
}
