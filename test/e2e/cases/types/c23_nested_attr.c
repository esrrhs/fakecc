// expect: 0
// C23 [[attr]] including a nested [[ ]] inside the attribute list.
package main;
[[gnu::unused]] int gx;
int main(void) {
    [[gnu::unused]] int x;
    [[a([[b]])]] int y;
    x = 1;
    y = 2;
    gx = x + y;
    return gx != 3;
}
