// expect: 9
// A block-scope `extern` function declaration must still lower its call site
// as a direct call, not as an indirect call through a data symbol.
package main;
int helper(int x) { return x + 4; }
int main() {
    extern int helper(int x);
    return helper(5);
}
