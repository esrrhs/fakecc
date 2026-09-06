// expect: 55
package main;

__thread int total;

void add_val(int n) {
    total += n;
}

int main() {
    total = 0;
    for (int i = 1; i <= 10; i++) {
        add_val(i);
    }
    return total;
}
