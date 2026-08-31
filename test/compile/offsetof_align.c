/* Test offsetof idiom with alignment padding. */
typedef unsigned long ul;
typedef unsigned char uc;
typedef int s32;
typedef char* ptr;

/* char[1025] followed by s32: 3 bytes padding => gid at 1028 */
struct Padded {
    char name[1025];
    s32 gid;
    char data[10250];
};

ul g_name = (ul)((uc *)&((struct Padded *)0)->name - (uc *)0);
ul g_gid  = (ul)((uc *)&((struct Padded *)0)->gid - (uc *)0);
ul g_data = (ul)((uc *)&((struct Padded *)0)->data - (uc *)0);

/* pointer member: arr[10] (40 bytes) then ptr (8 bytes) => p at 40 */
struct PtrS {
    s32 arr[10];
    ptr p;
    s32 tail;
};

ul g_parr = (ul)((uc *)&((struct PtrS *)0)->arr - (uc *)0);
ul g_pp   = (ul)((uc *)&((struct PtrS *)0)->p - (uc *)0);
ul g_ptail = (ul)((uc *)&((struct PtrS *)0)->tail - (uc *)0);
