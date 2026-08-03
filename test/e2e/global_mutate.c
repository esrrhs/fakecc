// expect: 100
package main;
int x = 10;
int y = 20;
int add() { return x + y; }
int main() {
    x = 100;
    return add() - 20;
}
