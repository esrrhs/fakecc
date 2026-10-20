/* Three-dimensional array and a pointer to a row. */
package main;
import runtime;

enum { L = 40 };
enum { STEPS = 80 };

static int grid[2][L][L][L];

int main(void) {
    int t, x, y, z, cur, nxt;
    unsigned sum;

    for (z = 0; z < L; z++)
        for (y = 0; y < L; y++)
            for (x = 0; x < L; x++)
                grid[0][z][y][x] = (x * 3 + y * 5 + z * 7) & 255;
    cur = 0;
    nxt = 1;
    for (t = 0; t < STEPS; t++) {
        for (z = 1; z < L - 1; z++) {
            for (y = 1; y < L - 1; y++) {
                int (*row)[L] = &grid[cur][z][y];
                for (x = 1; x < L - 1; x++) {
                    int v = row[0][x] * 4
                          + row[0][x - 1] + row[0][x + 1]
                          + grid[cur][z][y - 1][x] + grid[cur][z][y + 1][x]
                          + grid[cur][z - 1][y][x] + grid[cur][z + 1][y][x];
                    grid[nxt][z][y][x] = v & 1023;
                }
            }
        }
        cur = nxt;
        nxt = 1 - nxt;
    }
    sum = 0;
    for (z = 0; z < L; z++)
        for (y = 0; y < L; y++)
            for (x = 0; x < L; x++)
                sum += (unsigned)grid[cur][z][y][x];
    runtime.printf("stencil %u\n", sum);
    return 0;
}
