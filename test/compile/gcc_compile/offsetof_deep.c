/* Test deeply nested offsetof idiom &((T *)0)->a.b.c.d ... */
typedef unsigned long ul;
typedef unsigned char uc;
typedef int s32;

struct A { s32 a; s32 b; };
struct B { struct A a; s32 c; };
struct C { struct B b; s32 d; };
struct D { struct C c; s32 e; };
struct E { struct D d; s32 f; };

/* 4 levels deep */
ul g_b = (ul)((uc *)&((struct E *)0)->d.c.b.a.b - (uc *)0);
ul g_d = (ul)((uc *)&((struct E *)0)->d.c.d - (uc *)0);
ul g_f = (ul)((uc *)&((struct E *)0)->f - (uc *)0);

/* array of deep offsets (920928-4.c style) */
ul g_vec[] = {
    (ul)((uc *)&((struct E *)0)->d.c.b.a.a - (uc *)0),
    (ul)((uc *)&((struct E *)0)->d.c.b.a.b - (uc *)0),
    (ul)((uc *)&((struct E *)0)->d.c.b.c - (uc *)0),
    (ul)((uc *)&((struct E *)0)->d.c.d - (uc *)0),
    (ul)((uc *)&((struct E *)0)->d.e - (uc *)0),
    (ul)((uc *)&((struct E *)0)->f - (uc *)0),
};
