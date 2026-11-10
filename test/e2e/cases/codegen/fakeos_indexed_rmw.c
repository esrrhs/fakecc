// expect: 0
// fakeos: pending_sigs RMW and cfs_nr[cpu]++ must not use dead registers.
package main;
struct task { unsigned int mask; };
static struct task tasks[4];
static unsigned int counts[8];
static unsigned int idx(void) { return 2; }
int main(void) {
    unsigned int i;
    tasks[idx()].mask = 512;
    tasks[idx()].mask &= ~0u;
    if (tasks[2].mask != 512) return 41;
    tasks[idx()].mask |= 32768u;
    if (tasks[2].mask != 33280) return 42;
    tasks[idx()].mask &= ~(1u << 9);
    if (tasks[2].mask != 32768) return 43;
    for (i = 0; i < 8; i++) counts[i] = i;
    for (i = 0; i < 8; i++) counts[i]++;
    for (i = 0; i < 8; i++) if (counts[i] != i + 1) return 44;
    for (i = 0; i < 8; i++) counts[i] -= i;
    for (i = 0; i < 8; i++) if (counts[i] != 1) return 45;
    return 0;
}
