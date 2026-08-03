// expect: 244
// (char)300 wraps to 44; then 44 + 200 = 244.
package main;
int main() {
    int x = 300;
    char c = (char)x;
    int r = c;
    return r + 200;
}
