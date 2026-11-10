// expect: 0
// Variable-index width-4 (and width-2) RMW on file-scope objects must emit
// a real addl/xorl/andw memory op, not STORE_PTR of a stale register.
// Constant-index (non-indexed abs32) RMW must stay correct.
package main;

int arr[8];
int mat[16];
struct Vec { int x, y; };
struct Body { struct Vec v; int mass; };
struct Body bodies[4];

struct Row { int a, b, c; };
struct Pack { int val, aux; };
struct Row rows[4];
struct Pack packs[4];

short shorts[8];
int bits32[4];

int main(void) {
    int i, j;

    for (i = 0; i < 8; i = i + 1) arr[i] = i;
    for (i = 0; i < 16; i = i + 1) mat[i] = 0;
    for (i = 0; i < 4; i = i + 1) {
        bodies[i].v.x = 10;
        bodies[i].v.y = 1;
        bodies[i].mass = 8 + i;
        rows[i].a = 0;
        rows[i].b = 3;
        rows[i].c = 7;
        packs[i].val = 13;
        packs[i].aux = 1;
        bits32[i] = 0xFF;
    }
    for (i = 0; i < 8; i = i + 1) shorts[i] = (short)i;

    /* array[i] += imm */
    for (i = 0; i < 8; i = i + 1)
        arr[i] += 5;
    for (i = 0; i < 8; i = i + 1)
        if (arr[i] != i + 5) return 1;

    /* array[i] += reg */
    {
        int k = 3;
        for (i = 0; i < 8; i = i + 1)
            arr[i] += k;
    }
    for (i = 0; i < 8; i = i + 1)
        if (arr[i] != i + 8) return 2;

    /* array[i*N+j] += ... (matmul-like) */
    for (i = 0; i < 4; i = i + 1)
        for (j = 0; j < 4; j = j + 1)
            mat[i * 4 + j] += (i + 1) * (j + 1);
    if (mat[0] != 1) return 3;
    if (mat[3] != 4) return 4;
    if (mat[9] != 6) return 5;
    if (mat[15] != 16) return 6;

    /* struct field += with variable i */
    for (i = 0; i < 4; i = i + 1)
        bodies[i].v.x += bodies[i].mass - 8;
    if (bodies[0].v.x != 10) return 7;
    if (bodies[1].v.x != 11) return 8;
    if (bodies[3].v.x != 13) return 9;
    if (bodies[2].v.y != 1) return 10;

    /* rows[i].b ^= packs[i].val (align-like) */
    for (i = 0; i < 4; i = i + 1)
        rows[i].b ^= packs[i].val;
    for (i = 0; i < 4; i = i + 1)
        if (rows[i].b != 14) return 11;
    for (i = 0; i < 4; i = i + 1)
        if (packs[i].aux != 1) return 12;

    /* and / or / sub, variable index */
    for (i = 0; i < 4; i = i + 1)
        bits32[i] &= 0x0F;
    for (i = 0; i < 4; i = i + 1)
        if (bits32[i] != 0x0F) return 13;
    for (i = 0; i < 4; i = i + 1)
        bits32[i] |= 0x20;
    for (i = 0; i < 4; i = i + 1)
        if (bits32[i] != 0x2F) return 14;
    for (i = 0; i < 4; i = i + 1)
        bits32[i] -= 0x0F;
    for (i = 0; i < 4; i = i + 1)
        if (bits32[i] != 0x20) return 15;

    /* constant-index RMW (already a non-indexed abs32 form) */
    arr[3] += 5;
    if (arr[3] != 3 + 8 + 5) return 16;
    bodies[1].v.x += 2;
    if (bodies[1].v.x != 13) return 17;
    rows[2].b ^= 1;
    if (rows[2].b != 15) return 18;

    /* width-2 indexed RMW */
    for (i = 0; i < 8; i = i + 1)
        shorts[i] += 7;
    for (i = 0; i < 8; i = i + 1)
        if (shorts[i] != (short)(i + 7)) return 19;
    shorts[2] ^= 1;
    if (shorts[2] != 8) return 20;

    return 0;
}
