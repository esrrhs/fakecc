// expect: 25
package main;
int main() {
    int i = 0;
    int total = 0;
    while (i < 5) {
        int j = 0;
        while (j < 5) {
            total = total + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    return total;
}
