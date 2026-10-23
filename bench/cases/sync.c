/* __sync_* atomic builtins (single-threaded; stresses barriers/lowering). */
package main;
import runtime;

enum { N = 4000000 };

static unsigned counter;
static unsigned flag;
static unsigned slot;

int main(void) {
    unsigned i, acc, old, cur, nxt;

    counter = 0;
    flag = 1;
    slot = 0x55aa55aau;
    acc = 0;
    for (i = 0; i < N; i++) {
        old = __sync_fetch_and_add(&counter, (i & 3u) + 1u);
        acc += old;
        __sync_fetch_and_xor(&flag, i | 1u);
        __sync_fetch_and_or(&slot, i & 255u);
        __sync_fetch_and_and(&slot, ~(i & 7u));
        cur = flag;
        nxt = cur ^ 0x5a5a5a5au;
        if (__sync_bool_compare_and_swap(&flag, cur, nxt))
            acc ^= 1u;
        cur = counter;
        old = __sync_val_compare_and_swap(&counter, cur, cur + 1u);
        acc ^= old;
        __sync_synchronize();
    }
    runtime.printf("sync %u %u %u %u\n", acc, counter, flag, slot);
    return 0;
}
