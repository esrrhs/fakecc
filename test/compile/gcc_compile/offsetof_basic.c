/* Test the basic offsetof idiom &((T *)0)->member in global initializers.
   This is the classic, portable way to compute member offsets at compile time
   (GCC treats it as a compile-time integer constant). */
typedef unsigned long ul;
typedef unsigned char uc;
typedef int s32;

struct S { s32 a; s32 b; s32 c; };
struct Inner { s32 x; s32 y; };
struct Outer { struct Inner inner; s32 z; };

/* simple members */
ul g_a = (ul)((uc *)&((struct S *)0)->a - (uc *)0);
ul g_b = (ul)((uc *)&((struct S *)0)->b - (uc *)0);
ul g_c = (ul)((uc *)&((struct S *)0)->c - (uc *)0);

/* nested members */
ul g_ix = (ul)((uc *)&((struct Outer *)0)->inner.x - (uc *)0);
ul g_iy = (ul)((uc *)&((struct Outer *)0)->inner.y - (uc *)0);
ul g_oz = (ul)((uc *)&((struct Outer *)0)->z - (uc *)0);

/* without the trailing subtraction (bare address-of-member) */
ul g_bare = (ul)((uc *)&((struct S *)0)->b);
