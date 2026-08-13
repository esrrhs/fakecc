// expect: 5
package main;
int main() {
    const int x = 5;
    int y = 0;
    y = x + 1;   /* reading a const is fine */
    return y - 1;
}
