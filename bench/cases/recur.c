/* Recursive quicksort. The larger side is the loop, so depth stays logarithmic. */
package main;
import runtime;

enum { N = 80000 };
enum { PASSES = 6 };

static int data[N];

static void sort_range(int *a, int lo, int hi) {
    while (lo < hi) {
        int i, j, pivot, tmp;
        if (hi - lo < 12) {
            for (i = lo + 1; i <= hi; i++) {
                tmp = a[i];
                j = i;
                while (j > lo && a[j - 1] > tmp) {
                    a[j] = a[j - 1];
                    j--;
                }
                a[j] = tmp;
            }
            return;
        }
        pivot = a[lo + (hi - lo) / 2];
        i = lo;
        j = hi;
        while (i <= j) {
            while (a[i] < pivot) i++;
            while (a[j] > pivot) j--;
            if (i <= j) {
                tmp = a[i];
                a[i] = a[j];
                a[j] = tmp;
                i++;
                j--;
            }
        }
        if (j - lo < hi - i) {
            if (lo < j) sort_range(a, lo, j);
            lo = i;
        } else {
            if (i < hi) sort_range(a, i, hi);
            hi = j;
        }
    }
}

int main(void) {
    int pass, i;
    unsigned sum;
    unsigned long long acc;

    acc = 0;
    for (pass = 0; pass < PASSES; pass++) {
        unsigned s = 0x12345678u + (unsigned)pass * 97u;
        for (i = 0; i < N; i++) {
            s = s * 1664525u + 1013904223u;
            data[i] = (int)(s >> 8);
        }
        sort_range(data, 0, N - 1);
        sum = 0;
        for (i = 0; i < N; i += 17)
            sum += (unsigned)data[i];
        acc += sum;
    }
    runtime.printf("recur %llu\n", acc);
    return 0;
}
