#include <stdio.h>
#define PR(n, v) printf(n "=%llx\n", (unsigned long long)(v))
int g0 = (int)127ull;
signed char g1 = (signed char)7ull;
unsigned long long g2 = (unsigned long long)3ull;
unsigned int g3 = (unsigned int)7ull;
unsigned short g4 = (unsigned short)0ull;
long long g5 = (long long)255ull;
signed char ga0[3] = {(signed char)255ull, (signed char)127ull, (signed char)2ull};
int ga1[4] = {(int)256ull, (int)3ull, (int)0ull, (int)2ull};
static short f0(short p0, short p1, signed char p2) {
    unsigned char l0 = (unsigned char)((((int)1ull != (unsigned short)ga0[1]) || ((short)g4 > (unsigned short)g2)) ? (short)((unsigned int)(int)32767ull << ((unsigned)(unsigned int)255ull & 31)) : (long long)((unsigned long long)(unsigned int)127ull - (unsigned long long)(unsigned long long)g5));
    unsigned char l1 = (unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(unsigned short)127ull | (unsigned long long)(short)127ull) % ((unsigned int)(int)((unsigned int)(unsigned int)ga1[1] << ((unsigned)(unsigned int)ga1[2] & 31)) | 1u));
    g1 = (signed char)g2;
    if ((int)p2 > (signed char)((unsigned int)0 - (unsigned int)(unsigned int)g4)) {
        if ((signed char)((unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(int)g5) - (unsigned int)(unsigned char)((unsigned int)(short)p1 & (unsigned int)(int)3ull)) < (unsigned char)ga0[1]) {
            g0 = (int)(((short)((unsigned int)(int)((unsigned int)(unsigned int)g5 & (unsigned int)(signed char)g1) & (unsigned int)(short)((unsigned int)(int)p2 ^ (unsigned int)(unsigned short)ga0[2])) <= (long long)((unsigned long long)(unsigned char)((unsigned int)(int)32767ull * (unsigned int)(unsigned long long)7ull) >> ((unsigned)(unsigned int)g5 & 63))) ? (unsigned int)(((unsigned short)(~(unsigned int)(unsigned int)g1) < (long long)((((short)p2 < (short)0ull) && ((unsigned short)1ull == (int)p1)))) ? (int)((unsigned int)0 - (unsigned int)(unsigned short)ga0[0]) : (unsigned char)((unsigned int)(unsigned short)1ull >> ((unsigned)(unsigned int)ga0[0] & 31))) : (unsigned long long)((unsigned long long)0 - (unsigned long long)(int)(((unsigned char)256ull < (signed char)1ull) ? (signed char)100ull : (unsigned short)p0)));
            ga0[0] = (signed char)((unsigned int)(unsigned char)g4 >> ((unsigned)(unsigned int)(~(unsigned int)(signed char)((unsigned int)(long long)ga0[2] >> ((unsigned)(unsigned int)ga1[0] & 31))) & 31));
        } else {
            ga0[1] = (signed char)((unsigned int)(unsigned char)(~(unsigned int)(unsigned long long)((((unsigned long long)1ull <= (int)ga1[1]) && ((unsigned char)2ull <= (short)3ull)) ? (unsigned int)3ull : (int)g3)) ^ (unsigned int)(long long)((unsigned long long)(unsigned char)((unsigned int)0 - (unsigned int)(unsigned long long)255ull) ^ (unsigned long long)(unsigned short)((unsigned int)(unsigned long long)ga0[1] % ((unsigned int)(short)g4 | 1u))));
            p1 = (short)g3;
        }
        l1 = (unsigned char)(((unsigned int)((unsigned int)(long long)((unsigned long long)(unsigned int)g3 >> ((unsigned)(unsigned int)g0 & 63)) >> ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(int)100ull) & 31)) < (unsigned long long)((unsigned long long)(signed char)((unsigned int)(unsigned int)ga1[1] >> ((unsigned)(unsigned int)2ull & 31)) * (unsigned long long)(long long)(~(unsigned long long)(unsigned short)p0))) ? (int)((unsigned int)(short)(((signed char)ga0[0] == (unsigned char)l0)) >> ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(signed char)32767ull) & 31)) : (unsigned short)(~(unsigned int)(unsigned long long)((unsigned long long)(unsigned short)ga1[0] >> ((unsigned)(unsigned int)l0 & 63))));
        p1 = (short)(~(unsigned int)(int)((((unsigned int)1ull < (unsigned char)128ull) && (((signed char)ga0[2] != (long long)l0) || (((unsigned long long)g4) && ((signed char)128ull >= (unsigned char)ga1[1])))) ? (unsigned char)l0 : (unsigned int)g0));
    } else {
        for (int i0 = 0; i0 < 5; i0++) {
            ga1[2] = (int)((!((int)65535ull >= (int)((unsigned int)(long long)3ull + (unsigned int)(int)g2))) ? (unsigned int)((unsigned int)(unsigned char)((unsigned int)(unsigned short)256ull & (unsigned int)(unsigned long long)3ull) ^ (unsigned int)(unsigned long long)(((signed char)255ull > (short)l0) ? (unsigned long long)p1 : (signed char)g3)) : (signed char)((unsigned int)(signed char)((unsigned int)(unsigned char)ga1[0] >> ((unsigned)(unsigned int)g2 & 31)) | (unsigned int)(int)((unsigned int)(signed char)ga0[0] * (unsigned int)(unsigned long long)255ull)));
        }
        p0 = (short)(((unsigned char)((unsigned int)(signed char)((unsigned int)(unsigned char)ga1[1] + (unsigned int)(unsigned long long)g5) + (unsigned int)(unsigned int)g5) < (signed char)g3));
        g2 = (unsigned long long)(((((long long)1ull >= (long long)108ull) && ((signed char)ga0[2] >= (unsigned int)l1)) && (!((unsigned int)g0 > (unsigned int)3230345373ull))) ? (short)1ull : (short)g3);
    }
    for (int i1 = 0; i1 < 1; i1++) {
        switch ((int)((int)g2) & 7) {
        case 0:
            g5 = (long long)g0;
            break;
        case 1:
            g0 = (int)g3;
            break;
        case 3:
            g4 = (unsigned short)p0;
            break;
        case 4:
            ga1[2] = (int)((unsigned int)(unsigned int)p1 | (unsigned int)(long long)l0);
        default:
            ga0[0] = (signed char)((unsigned int)(unsigned char)((unsigned int)(int)((unsigned int)0 - (unsigned int)(unsigned int)g4) % ((unsigned int)(unsigned char)((((unsigned short)ga0[1] > (long long)255ull) || ((int)g1 == (unsigned int)p2)) ? (short)p0 : (signed char)l0) | 1u)) / ((unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(unsigned short)(((short)ga1[2] != (short)g5) ? (unsigned int)ga1[0] : (short)p2)) | 1u));
        }
        for (int i2 = 0; i2 < 5; i2++) {
            p1 = (short)((unsigned int)(short)((unsigned int)(unsigned short)((unsigned int)(unsigned short)0ull % ((unsigned int)(unsigned long long)ga0[1] | 1u)) & (unsigned int)(short)(((unsigned long long)32767ull < (long long)ga0[1]))) & (unsigned int)(signed char)((unsigned int)(unsigned int)((unsigned int)(signed char)g3 >> ((unsigned)(unsigned int)ga1[1] & 31)) + (unsigned int)(long long)ga1[3]));
            ga0[2] = (signed char)(((signed char)((unsigned int)(signed char)p1 >> ((unsigned)(unsigned int)((unsigned int)(unsigned long long)1ull + (unsigned int)(unsigned short)g4) & 31)) <= (int)((unsigned int)(unsigned char)((unsigned int)(unsigned short)170ull & (unsigned int)(unsigned char)ga0[1]) * (unsigned int)(unsigned int)((unsigned int)(unsigned short)255ull | (unsigned int)(short)255ull))) ? (unsigned int)((unsigned int)(short)65535ull / ((unsigned int)(unsigned char)((unsigned int)(long long)ga0[0] % ((unsigned int)(unsigned long long)g3 | 1u)) | 1u)) : (unsigned char)p2);
            g1 = (signed char)((unsigned int)(unsigned short)(~(unsigned int)(long long)p1) << ((unsigned)(unsigned int)(((unsigned short)127ull < (unsigned int)(((((((long long)p0 > (short)g2) && ((unsigned int)3818581611ull != (signed char)7ull)) || (((signed char)ga1[3] != (unsigned long long)p0) && ((int)1ull < (short)256ull))) && ((unsigned long long)g4 != (signed char)ga0[2])) || ((long long)g2 > (short)p0)) ? (signed char)ga0[2] : (signed char)65535ull)) ? (unsigned int)((unsigned int)(long long)ga1[1] ^ (unsigned int)(int)ga0[2]) : (long long)((unsigned long long)0 - (unsigned long long)(signed char)63ull)) & 31));
        }
        switch ((int)((int)(~(unsigned int)(signed char)(~(unsigned int)(unsigned char)p1))) & 7) {
        case 4:
            g3 = (unsigned int)((unsigned int)(short)l0 & (unsigned int)(signed char)((unsigned int)(unsigned char)((unsigned int)(unsigned char)((unsigned int)(signed char)2ull | (unsigned int)(unsigned long long)15ull) << ((unsigned)(unsigned int)ga0[0] & 31)) | (unsigned int)(long long)((unsigned long long)(unsigned int)((((unsigned long long)g5 == (unsigned long long)p1) && ((unsigned int)g5 >= (unsigned long long)2ull))) / ((unsigned long long)(unsigned short)(~(unsigned int)(unsigned short)g5) | 1u))));
            break;
        case 5:
            g0 = (int)((unsigned int)(unsigned char)(~(unsigned int)(signed char)ga1[1]) * (unsigned int)(unsigned long long)((unsigned long long)(unsigned char)7ull ^ (unsigned long long)(signed char)p0));
            break;
        default:
            g5 = (long long)8970545083274231302ull;
        }
    }
    g2 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(short)g2 / ((unsigned long long)(int)((unsigned int)(int)l0 << ((unsigned)(unsigned int)7ull & 31)) | 1u)) * (unsigned long long)(unsigned long long)g2);
    g4 = (unsigned short)(~(unsigned int)(int)((unsigned int)(unsigned short)((unsigned int)(unsigned int)2146510333ull & (unsigned int)(long long)1ull) + (unsigned int)(short)p2));
    g3 = (unsigned int)((unsigned int)(unsigned char)((unsigned int)(unsigned int)l0 | (unsigned int)(unsigned int)2ull) + (unsigned int)(int)((unsigned int)(int)g2 * (unsigned int)(signed char)((unsigned int)(int)l1 << ((unsigned)(unsigned int)32767ull & 31))));
    return (short)2ull;
}

static signed char f1(int p0) {
    signed char l0 = (signed char)((unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(int)100ull) / ((unsigned int)(unsigned char)f0((short)255ull, (short)256ull, (signed char)127ull) | 1u));
    int l1 = (int)g3;
    unsigned int l2 = (unsigned int)g0;
    l0 = (signed char)(~(unsigned int)(long long)1ull);
    for (int i0 = 0; i0 < 5; i0++) {
        g2 = (unsigned long long)(~(unsigned long long)(unsigned long long)(~(unsigned long long)(unsigned short)f0((short)255ull, (short)g4, (signed char)ga1[1])));
    }
    ga0[0] = (signed char)(~(unsigned int)(long long)(((unsigned short)((unsigned int)(unsigned int)g0 * (unsigned int)(int)256ull) != (int)ga0[0])));
    ga0[0] = (signed char)((unsigned int)(long long)((unsigned long long)(long long)((unsigned long long)(unsigned int)g4 & (unsigned long long)(int)ga1[0]) | (unsigned long long)(unsigned int)((unsigned int)(long long)256ull - (unsigned int)(unsigned int)65535ull)) << ((unsigned)(unsigned int)((unsigned int)(int)((unsigned int)0 - (unsigned int)(long long)78ull) | (unsigned int)(signed char)((unsigned int)(unsigned short)p0 * (unsigned int)(unsigned int)ga1[0])) & 31));
    if ((unsigned int)(~(unsigned int)(unsigned int)f0((short)127ull, (short)ga1[2], (signed char)ga1[0])) <= (int)(~(unsigned int)(unsigned char)f0((short)g0, (short)g4, (signed char)g2))) {
        if ((int)((unsigned int)0 - (unsigned int)(long long)((unsigned long long)(signed char)l2 << ((unsigned)(unsigned int)128ull & 63))) < (signed char)f0((short)(((int)g0 != (signed char)l2)), (short)l2, (signed char)((((signed char)g1 > (unsigned long long)128ull) && (!((int)ga1[3] > (unsigned int)p0))) ? (int)l1 : (unsigned long long)g5))) {
            g4 = (unsigned short)((unsigned int)(unsigned int)((unsigned int)(unsigned int)(~(unsigned int)(unsigned int)((unsigned int)(short)g4 & (unsigned int)(long long)g1)) ^ (unsigned int)(unsigned short)((unsigned int)0 - (unsigned int)(unsigned int)f0((short)g2, (short)15ull, (signed char)ga0[1]))) + (unsigned int)(long long)f0((short)((unsigned int)(int)f0((short)3ull, (short)0ull, (signed char)1ull) / ((unsigned int)(long long)(((int)g3 < (unsigned short)ga1[0])) | 1u)), (short)((unsigned int)(unsigned int)f0((short)g3, (short)g5, (signed char)p0) - (unsigned int)(unsigned short)(((long long)256ull) ? (long long)127ull : (long long)l2)), (signed char)((((unsigned long long)128ull >= (unsigned int)24ull) && (((signed char)255ull >= (unsigned short)l0) || ((((unsigned long long)p0) && ((unsigned char)3ull == (long long)ga0[0])) || ((unsigned int)g0 > (unsigned short)2ull)))))));
            ga1[3] = (int)((unsigned int)(unsigned int)((unsigned int)(unsigned char)((unsigned int)(unsigned char)ga0[2] ^ (unsigned int)(signed char)1ull) + (unsigned int)(signed char)((unsigned int)(int)0ull & (unsigned int)(short)l1)) & (unsigned int)(unsigned long long)7ull);
            g0 = (int)(((long long)g2 != (unsigned int)((unsigned int)(unsigned int)(((short)p0 == (int)3ull) ? (signed char)73ull : (int)g5) | (unsigned int)(signed char)((unsigned int)(long long)3ull | (unsigned int)(short)0ull))) ? (short)(((unsigned int)3ull == (unsigned int)g1) ? (signed char)g0 : (unsigned int)((((signed char)127ull != (unsigned long long)1ull) && ((unsigned int)l1 > (unsigned int)15ull)) ? (signed char)l0 : (unsigned short)g2)) : (short)((unsigned int)(long long)((unsigned long long)(signed char)255ull + (unsigned long long)(unsigned long long)15ull) * (unsigned int)(unsigned int)((unsigned int)(signed char)ga0[1] << ((unsigned)(unsigned int)96ull & 31))));
        } else {
            p0 = (int)((unsigned int)(long long)g4 / ((unsigned int)(short)32767ull | 1u));
            ga0[2] = (signed char)l2;
            ga0[0] = (signed char)l0;
        }
        p0 = (int)f0((short)((unsigned int)(long long)(((short)((unsigned int)0 - (unsigned int)(unsigned long long)15ull) < (unsigned char)((unsigned int)(long long)65535ull % ((unsigned int)(unsigned int)15ull | 1u))) ? (unsigned short)f0((short)7ull, (short)32767ull, (signed char)1ull) : (int)f0((short)ga1[1], (short)g1, (signed char)32767ull)) / ((unsigned int)(int)((unsigned int)(unsigned long long)(((((unsigned int)3ull) || (((short)l1 != (unsigned int)l2) || (((short)l2 < (unsigned short)2ull) && ((long long)128ull >= (unsigned int)l0)))) || ((((signed char)g2 <= (short)150ull) && ((((long long)2ull >= (unsigned char)ga1[3]) && ((short)2ull)) && ((unsigned char)15ull < (signed char)g0))) || ((unsigned char)g2 <= (int)255ull)))) - (unsigned int)(int)g3) | 1u)), (short)15ull, (signed char)((unsigned int)(short)((unsigned int)(short)((unsigned int)(short)ga0[2] - (unsigned int)(short)g1) / ((unsigned int)(unsigned char)l1 | 1u)) - (unsigned int)(unsigned char)((unsigned int)(int)f0((short)l1, (short)2ull, (signed char)g2) << ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(int)p0) & 31))));
        { int w1 = 1; while (w1 > 0) {
            g1 = (signed char)(((signed char)ga0[0] > (unsigned int)ga0[2]) ? (int)ga1[0] : (int)l0);
            w1--;
        } }
    } else {
        ga1[3] = (int)((unsigned int)(long long)(((short)((unsigned int)(short)2ull & (unsigned int)(unsigned char)3ull) <= (short)((unsigned int)(long long)ga1[0] >> ((unsigned)(unsigned int)0ull & 31))) ? (short)(~(unsigned int)(short)0ull) : (short)32767ull) + (unsigned int)(unsigned int)((unsigned int)(signed char)((unsigned int)(unsigned long long)32767ull & (unsigned int)(unsigned int)ga0[1]) | (unsigned int)(unsigned char)((!((((short)g2 <= (signed char)135ull) && ((unsigned int)g4 != (unsigned long long)3ull)) && ((int)256ull))) ? (long long)p0 : (unsigned long long)g0)));
    }
    l0 = (signed char)((unsigned int)(short)174ull | (unsigned int)(unsigned int)256ull);
    return (signed char)((unsigned int)(unsigned long long)(((!(((unsigned short)p0 == (long long)128ull) || ((signed char)ga1[2]))) && ((unsigned int)3ull >= (short)g4))) ^ (unsigned int)(unsigned char)(~(unsigned int)(unsigned long long)127ull));
}

int main(void) {
    unsigned char m0 = (unsigned char)255ull;
    short m1 = (short)0ull;
    unsigned int m2 = (unsigned int)100ull;
    { int w0 = 4; while (w0 > 0) {
        g4 = (unsigned short)((unsigned int)(int)m0 * (unsigned int)(short)ga1[3]);
        w0--;
    } }
    { int w1 = 3; while (w1 > 0) {
        m0 = (unsigned char)((unsigned int)(unsigned short)m0 % ((unsigned int)(unsigned int)g3 | 1u));
        w1--;
    } }
    m2 = (unsigned int)f1((int)f0((short)((unsigned int)(int)((unsigned int)(unsigned short)256ull + (unsigned int)(unsigned int)g3) % ((unsigned int)(unsigned char)((unsigned int)(signed char)ga1[2] >> ((unsigned)(unsigned int)m1 & 31)) | 1u)), (short)((unsigned int)(unsigned long long)ga1[0] & (unsigned int)(short)((unsigned int)(unsigned short)g1 % ((unsigned int)(unsigned int)m2 | 1u))), (signed char)m1));
    { int w2 = 1; while (w2 > 0) {
        for (int i3 = 0; i3 < 5; i3++) {
            ga1[1] = (int)(~(unsigned int)(unsigned long long)f1((int)((((unsigned int)m2 >= (signed char)ga1[3]) && ((long long)255ull != (short)65535ull)) ? (unsigned char)g2 : (unsigned long long)256ull)));
            m2 = (unsigned int)((unsigned int)(unsigned long long)3ull + (unsigned int)(short)i3);
        }
        w2--;
    } }
    g3 = (unsigned int)((unsigned int)0 - (unsigned int)(signed char)g0);
    g1 = (signed char)((unsigned int)(int)((!((short)0ull < (unsigned long long)g0))) & (unsigned int)(signed char)0ull);
    ga0[2] = (signed char)((unsigned int)0 - (unsigned int)(long long)(~(unsigned long long)(long long)15ull));
    PR("g0", g0);
    PR("g1", g1);
    PR("g2", g2);
    PR("g3", g3);
    PR("g4", g4);
    PR("g5", g5);
    PR("m0", m0);
    PR("m1", m1);
    PR("m2", m2);
    PR("ga0_0", ga0[0]);
    PR("ga0_1", ga0[1]);
    PR("ga0_2", ga0[2]);
    PR("ga1_0", ga1[0]);
    PR("ga1_1", ga1[1]);
    PR("ga1_2", ga1[2]);
    PR("ga1_3", ga1[3]);
    return 0;
}

