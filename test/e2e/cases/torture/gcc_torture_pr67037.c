// expect: 0
package main;

typedef unsigned short short_unsigned;

long (*extfunc)();

static void lstrcpynW(short_unsigned *d, const short_unsigned *s, int n)
{
    int count = n;

    while ((count > 1) && *s)
    {
        count--;
        *d++ = *s++;
    }
    if (count) *d = 0;
}

int __attribute__((noinline))
badfunc(int u0, int u1, int u2, int u3,
  short_unsigned *fsname, unsigned int fsname_len)
{
    static const short_unsigned ntfsW[] = {'N','T','F','S',0};
    char superblock[5348];
    int ret = 0;
    short_unsigned *p;

    if (extfunc())
        return 0;
    p = (short_unsigned *)extfunc();
    if (p != 0)
        goto done;

    ((void(*)(char*))extfunc)(superblock);

    lstrcpynW(fsname, ntfsW, fsname_len);

    ret = 1;
done:
    return ret;
}

static long f()
{
    return 0;
}

int main()
{
    short_unsigned buf[6];
    extfunc = f;
    return !badfunc(0, 0, 0, 0, buf, 6);
}
