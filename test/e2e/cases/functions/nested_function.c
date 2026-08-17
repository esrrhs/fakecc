// expect: 42
package main;

int main() {
    int mul(int a, int b) {
        return a * b;
    }

    int add(int a, int b) {
        return a + b;
    }

    return add(mul(6, 6), 6);
}
