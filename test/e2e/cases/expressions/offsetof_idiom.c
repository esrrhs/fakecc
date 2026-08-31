// expect: 0
package main;

typedef unsigned long ul;
typedef unsigned char uc;
typedef int s32;
typedef char* ptr;

/* ---- basic struct ---- */
struct S1 {
    s32 a;
    s32 b;
    s32 c;
};

/* ---- nested structs ---- */
struct Inner {
    s32 x;
    s32 y;
};
struct Mid {
    struct Inner inner;
    s32 z;
};
struct Outer {
    struct Mid mid;
    s32 w;
};

/* ---- struct with alignment padding ---- */
struct Padded {
    char name[1025];
    s32 gid;
    char data[10250];
};

/* ---- struct with pointer ---- */
struct PtrStruct {
    s32 arr[10];
    ptr p;
    s32 tail;
};

/* ---- deep nesting (4 levels) ---- */
struct L1 { s32 a; s32 b; };
struct L2 { struct L1 l1; s32 c; };
struct L3 { struct L2 l2; s32 d; };
struct L4 { struct L3 l3; s32 e; };

/* ==== global initializers using the offsetof idiom ==== */

/* simple members */
ul g_simple_a = (ul)((uc *)&((struct S1 *)0)->a - (uc *)0);
ul g_simple_b = (ul)((uc *)&((struct S1 *)0)->b - (uc *)0);
ul g_simple_c = (ul)((uc *)&((struct S1 *)0)->c - (uc *)0);

/* nested members */
ul g_nested_inner_x = (ul)((uc *)&((struct Outer *)0)->mid.inner.x - (uc *)0);
ul g_nested_inner_y = (ul)((uc *)&((struct Outer *)0)->mid.inner.y - (uc *)0);
ul g_nested_z = (ul)((uc *)&((struct Outer *)0)->mid.z - (uc *)0);
ul g_nested_w = (ul)((uc *)&((struct Outer *)0)->w - (uc *)0);

/* alignment padding: char[1025] then s32 => 1025 + 3 padding = 1028 */
ul g_padded_name = (ul)((uc *)&((struct Padded *)0)->name - (uc *)0);
ul g_padded_gid = (ul)((uc *)&((struct Padded *)0)->gid - (uc *)0);
ul g_padded_data = (ul)((uc *)&((struct Padded *)0)->data - (uc *)0);

/* pointer member */
ul g_ptr_p = (ul)((uc *)&((struct PtrStruct *)0)->p - (uc *)0);
ul g_ptr_tail = (ul)((uc *)&((struct PtrStruct *)0)->tail - (uc *)0);

/* deep nesting */
ul g_deep_b = (ul)((uc *)&((struct L4 *)0)->l3.l2.l1.b - (uc *)0);
ul g_deep_c = (ul)((uc *)&((struct L4 *)0)->l3.l2.c - (uc *)0);
ul g_deep_d = (ul)((uc *)&((struct L4 *)0)->l3.d - (uc *)0);
ul g_deep_e = (ul)((uc *)&((struct L4 *)0)->e - (uc *)0);

/* array of offsets (the 920928-4.c pattern) */
ul g_offsets[] = {
    (ul)((uc *)&((struct Outer *)0)->mid.inner.x - (uc *)0),
    (ul)((uc *)&((struct Outer *)0)->mid.inner.y - (uc *)0),
    (ul)((uc *)&((struct Outer *)0)->mid.z - (uc *)0),
    (ul)((uc *)&((struct Outer *)0)->w - (uc *)0),
};

int main(void) {
    /* S1: a=0, b=4, c=8 */
    if (g_simple_a != 0) return 1;
    if (g_simple_b != 4) return 2;
    if (g_simple_c != 8) return 3;

    /* Outer: mid.inner.x=0, mid.inner.y=4, mid.z=8, w=12 */
    if (g_nested_inner_x != 0) return 4;
    if (g_nested_inner_y != 4) return 5;
    if (g_nested_z != 8) return 6;
    if (g_nested_w != 12) return 7;

    /* Padded: name=0, gid=1028 (1025+3 pad), data=1032 (1028+4) */
    if (g_padded_name != 0) return 8;
    if (g_padded_gid != 1028) return 9;
    if (g_padded_data != 1032) return 10;

    /* PtrStruct: arr=0, p=40 (10*4), tail=48 (40+8) */
    if (g_ptr_p != 40) return 11;
    if (g_ptr_tail != 48) return 12;

    /* L4 deep: l3.l2.l1.b=4, l3.l2.c=8, l3.d=12, e=16 */
    if (g_deep_b != 4) return 13;
    if (g_deep_c != 8) return 14;
    if (g_deep_d != 12) return 15;
    if (g_deep_e != 16) return 16;

    /* array form */
    if (g_offsets[0] != 0) return 17;
    if (g_offsets[1] != 4) return 18;
    if (g_offsets[2] != 8) return 19;
    if (g_offsets[3] != 12) return 20;

    return 0;
}
