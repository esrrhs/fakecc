/* Naive i32 matrix multiply. Stresses nested loops, addressing, and MAC. */
package main;
import runtime;

enum { N = 220 };

static int A[N * N];
static int B[N * N];
static int C[N * N];

int main(void) {
    int i, j, k, rounds;
    unsigned long long sum;

    for (i = 0; i < N * N; i++) {
        A[i] = (i * 17 + 3) & 0xff;
        B[i] = (i * 13 + 7) & 0xff;
        C[i] = 0;
    }

    /* Enough repeats that gcc -O2 still spends tens of milliseconds. */
    for (rounds = 0; rounds < 12; rounds++) {
        for (i = 0; i < N; i++) {
            for (k = 0; k < N; k++) {
                int aik = A[i * N + k];
                for (j = 0; j < N; j++)
                    C[i * N + j] += aik * B[k * N + j];
            }
        }
    }

    sum = 0;
    for (i = 0; i < N * N; i++)
        sum += (unsigned int)C[i];
    runtime.printf("matmul %llu\n", sum);
    return 0;
}
