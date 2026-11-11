// expect: 0
// Width-1 indexed RMW (sieve-like bits[i>>3] &= ~(1u<<(i&7))) inside a
// helper so i is SSA-promoted at -O1.  Remat of the mask/index must not
// use an unset RSI or the raw argument as the mask.
package main;

unsigned char bits[32];

void clear_bit(int i) {
    bits[i >> 3] &= (unsigned char)~(1u << (i & 7));
}

int bit_is_set(int i) {
    return (bits[i >> 3] & (unsigned char)(1u << (i & 7))) != 0;
}

int main(void) {
    int i;
    for (i = 0; i < 32; i = i + 1)
        bits[i] = 0xFF;

    for (i = 0; i < 200; i = i + 2)
        clear_bit(i);

    for (i = 0; i < 200; i = i + 1) {
        if ((i & 1) == 0) {
            if (bit_is_set(i)) return 1;
        } else {
            if (!bit_is_set(i)) return 2;
        }
    }
    /* bits 200..255 were not touched */
    if (bits[25] != 0xFF) return 3;
    if (bits[31] != 0xFF) return 4;
    return 0;
}
