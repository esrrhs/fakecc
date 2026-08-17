// expect: 42
package main;

int main() {
    int x = ({
        int a = 10;
        int b = 32;
        a + b;
    });

    int y = ({
        int sum = 0;
        for (int i = 1; i <= 4; i++) {
            sum += ({ int sq = i * i; sq; });
        }
        sum;
    });

    if (y != 30) return 1;

    return x;
}
