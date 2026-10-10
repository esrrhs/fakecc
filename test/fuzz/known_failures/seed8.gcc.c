#include <stdio.h>
#define PR(n, v) printf(n "=%llx\n", (unsigned long long)(v))
unsigned int g0 = (unsigned int)127ull;
long long g1 = (long long)127ull;
short g2 = (short)49ull;
unsigned short g3 = (unsigned short)1ull;
signed char g4 = (signed char)65535ull;
unsigned char g5 = (unsigned char)7ull;
short g6 = (short)127ull;
unsigned short ga0[7] = {(unsigned short)100ull, (unsigned short)256ull, (unsigned short)16931ull, (unsigned short)255ull, (unsigned short)1ull, (unsigned short)15ull, (unsigned short)1ull};
unsigned short ga1[6] = {(unsigned short)256ull, (unsigned short)3ull, (unsigned short)2ull, (unsigned short)128ull, (unsigned short)127ull, (unsigned short)2ull};
static short f0(unsigned long long p0, int p1, unsigned int p2, long long p3) {
    unsigned char l0 = (unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(unsigned int)p1 - (unsigned long long)(unsigned char)ga0[2]) * (unsigned int)(unsigned short)((unsigned int)(int)7ull - (unsigned int)(long long)42ull));
    g4 = (signed char)g5;
    switch ((int)((int)g2) & 7) {
    case 3:
        ga0[0] = (unsigned short)15ull;
        break;
    case 6:
        { int w0 = 4; while (w0 > 0) {
            g1 = (long long)1ull;
            w0--;
        } }
    case 7:
        g0 = (unsigned int)g0;
    default:
        switch ((int)((int)((unsigned int)0 - (unsigned int)(short)p0)) & 7) {
        case 1:
            p0 = (unsigned long long)((unsigned long long)(unsigned long long)((((unsigned long long)g5 >= (unsigned short)1ull) && ((signed char)p2 < (unsigned int)ga0[5]))) ^ (unsigned long long)(unsigned short)((unsigned int)(unsigned short)75ull & (unsigned int)(unsigned char)g6));
            break;
        case 7:
            l0 = (unsigned char)(~(unsigned int)(unsigned int)(((int)((unsigned int)(unsigned int)216ull & (unsigned int)(int)100ull)) ? (unsigned int)(((long long)9757105672052003166ull < (int)2139932401ull) ? (int)ga0[4] : (int)ga1[4]) : (int)g6));
            break;
        default:
            p1 = (int)((unsigned int)0 - (unsigned int)(unsigned short)(~(unsigned int)(unsigned char)((((signed char)g2) || ((unsigned short)g1 < (unsigned long long)p3)))));
        }
    }
    return (short)g1;
}

static unsigned int f1(unsigned int p0, unsigned char p1) {
    unsigned char l0 = (unsigned char)(((unsigned char)(~(unsigned int)(unsigned int)15ull) < (unsigned char)(((short)100ull != (unsigned char)7ull) ? (short)0ull : (signed char)g2)));
    if ((unsigned short)((unsigned int)(short)((unsigned int)(unsigned short)g2 * (unsigned int)(short)g0) << ((unsigned)(unsigned int)((unsigned int)(long long)ga1[4] % ((unsigned int)(unsigned int)p0 | 1u)) & 31)) > (short)(((short)((unsigned int)(int)ga1[1] ^ (unsigned int)(unsigned long long)g1) > (long long)ga1[2]))) {
        for (int i0 = 0; i0 < 5; i0++) {
            ga0[3] = (unsigned short)((unsigned int)(unsigned char)((unsigned int)(unsigned int)438630247ull >> ((unsigned)(unsigned int)((unsigned int)(unsigned int)ga1[2] * (unsigned int)(signed char)i0) & 31)) & (unsigned int)(unsigned int)ga1[3]);
        }
        g4 = (signed char)(~(unsigned int)(unsigned long long)(((int)127ull != (unsigned char)p0) ? (unsigned int)1ull : (unsigned int)p0));
    } else {
        l0 = (unsigned char)((unsigned int)(unsigned char)(((short)g4 <= (unsigned char)113ull)) << ((unsigned)(unsigned int)((unsigned int)(signed char)l0 + (unsigned int)(int)g0) & 31));
        g5 = (unsigned char)f0((unsigned long long)ga0[4], (int)p0, (unsigned int)ga1[0], (long long)g2);
        g5 = (unsigned char)((unsigned int)(unsigned short)((unsigned int)(unsigned int)((unsigned int)(int)((unsigned int)(long long)g3 << ((unsigned)(unsigned int)256ull & 31)) & (unsigned int)(unsigned long long)((unsigned long long)0 - (unsigned long long)(unsigned short)g1)) & (unsigned int)(long long)((unsigned long long)(unsigned long long)l0 >> ((unsigned)(unsigned int)128ull & 63))) ^ (unsigned int)(unsigned int)((unsigned int)(unsigned short)((!((short)g4 < (unsigned long long)3ull))) - (unsigned int)(long long)f0((unsigned long long)((unsigned long long)(long long)p0 + (unsigned long long)(long long)p0), (int)((!(!((signed char)ga1[2] != (unsigned long long)ga0[4])))), (unsigned int)((unsigned int)(int)3ull & (unsigned int)(unsigned char)32767ull), (long long)((unsigned long long)(unsigned long long)g4 << ((unsigned)(unsigned int)g0 & 63)))));
    }
    g3 = (unsigned short)((unsigned int)(unsigned long long)g2 >> ((unsigned)(unsigned int)ga0[2] & 31));
    for (int i1 = 0; i1 < 5; i1++) {
        { int w2 = 5; while (w2 > 0) {
            g4 = (signed char)f0((unsigned long long)((unsigned long long)(int)1059307969ull & (unsigned long long)(unsigned long long)w2), (int)((unsigned int)(unsigned char)g4 << ((unsigned)(unsigned int)128ull & 31)), (unsigned int)255ull, (long long)((unsigned long long)(long long)128ull - (unsigned long long)(unsigned long long)l0));
            w2--;
        } }
        g4 = (signed char)((unsigned int)(long long)((unsigned long long)(signed char)((unsigned int)(long long)g6 % ((unsigned int)(unsigned long long)ga1[1] | 1u)) - (unsigned long long)(unsigned char)((unsigned int)(int)i1 - (unsigned int)(short)34187ull)) & (unsigned int)(unsigned int)((unsigned int)(unsigned int)f0((unsigned long long)255ull, (int)1ull, (unsigned int)ga0[5], (long long)128ull) / ((unsigned int)(long long)g1 | 1u)));
        g6 = (short)((unsigned int)(unsigned char)ga1[5] + (unsigned int)(long long)0ull);
    }
    return (unsigned int)((unsigned int)(unsigned long long)((unsigned long long)(signed char)g1 + (unsigned long long)(unsigned int)15ull) + (unsigned int)(unsigned int)(((unsigned char)256ull <= (unsigned short)100ull)));
}

static signed char f2(unsigned short p0) {
    long long l0 = (long long)((unsigned long long)(signed char)f1((unsigned int)g2, (unsigned char)ga1[5]) << ((unsigned)(unsigned int)((unsigned int)(int)g0 + (unsigned int)(signed char)g4) & 63));
    g3 = (unsigned short)255ull;
    g4 = (signed char)(((((unsigned long long)ga0[6] == (unsigned int)ga1[4]) && (((int)7ull < (signed char)g6) || ((unsigned short)ga0[1]))) || (((!(((((((unsigned long long)g4) && ((long long)g1 >= (unsigned int)p0)) || ((((unsigned short)ga0[3] >= (unsigned int)65535ull) && ((!((long long)p0 > (signed char)g3)) && (!(!((short)ga0[6] <= (unsigned short)ga1[4]))))) && (((!((int)127ull < (unsigned int)ga1[5])) && ((short)g0 != (unsigned char)7ull)) && ((signed char)ga0[6] >= (signed char)15ull)))) && ((signed char)15ull > (long long)1ull)) && ((unsigned short)g6 < (unsigned long long)82ull)) || ((signed char)109ull < (unsigned short)203ull))) && (((short)28944ull != (unsigned char)ga1[2]) || ((unsigned int)1577427232ull > (unsigned long long)g3))) || ((unsigned short)g0 < (long long)ga1[3]))));
    for (int i0 = 0; i0 < 6; i0++) {
        g6 = (short)((unsigned int)(unsigned char)((unsigned int)(long long)((unsigned long long)(unsigned short)128ull << ((unsigned)(unsigned int)ga0[6] & 63)) * (unsigned int)(short)3ull) % ((unsigned int)(unsigned long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned long long)2ull | (unsigned int)(long long)g0) * (unsigned long long)(unsigned char)ga1[4]) | 1u));
    }
    ga0[6] = (unsigned short)((unsigned int)(unsigned long long)f0((unsigned long long)((unsigned long long)(long long)g2 | (unsigned long long)(signed char)p0), (int)1ull, (unsigned int)((unsigned int)(int)g2 + (unsigned int)(long long)128ull), (long long)p0) * (unsigned int)(short)((unsigned int)(unsigned long long)f1((unsigned int)7ull, (unsigned char)2ull) >> ((unsigned)(unsigned int)((unsigned int)(int)255ull * (unsigned int)(short)p0) & 31)));
    g3 = (unsigned short)((unsigned int)(unsigned short)(((short)((unsigned int)(unsigned char)65535ull - (unsigned int)(unsigned short)f0((unsigned long long)g5, (int)g6, (unsigned int)100ull, (long long)g4)) < (long long)p0)) * (unsigned int)(int)ga0[0]);
    if (((short)((unsigned int)(short)32767ull ^ (unsigned int)(short)65535ull)) || ((signed char)((unsigned int)(short)g6 & (unsigned int)(short)ga1[1]) > (unsigned int)ga1[0])) {
        if ((short)((unsigned int)(long long)((((signed char)g5 > (short)219ull) && ((((unsigned char)2ull >= (short)0ull) || ((signed char)g2)) && ((signed char)g1 >= (short)ga1[2]))) ? (unsigned long long)g5 : (unsigned int)127ull) ^ (unsigned int)(short)g6) >= (short)127ull) {
            g5 = (unsigned char)((unsigned int)(unsigned char)(((short)g3 > (signed char)g3) ? (unsigned short)ga0[4] : (int)100ull) | (unsigned int)(unsigned char)((unsigned int)(unsigned char)g3 >> ((unsigned)(unsigned int)1ull & 31)));
            g4 = (signed char)(((long long)g4 == (long long)(((((unsigned int)7ull < (unsigned long long)g1) || ((((!((short)g0)) || ((unsigned short)65535ull != (unsigned char)g0)) && ((unsigned long long)g6 <= (unsigned long long)g3)) || (((unsigned int)p0 == (signed char)ga0[2]) && (!((int)g4 > (unsigned char)2ull))))) || ((int)g1 != (short)g6)))) ? (signed char)g1 : (unsigned short)g2);
        } else {
            ga1[0] = (unsigned short)((unsigned int)(unsigned int)((unsigned int)(unsigned short)((unsigned int)(unsigned char)ga0[4] * (unsigned int)(int)g2) * (unsigned int)(signed char)((((unsigned short)15ull != (unsigned char)p0) && (((long long)ga1[5] < (long long)127ull) && (((int)ga1[4]) && (((((unsigned short)1ull > (unsigned short)g6) || ((((unsigned char)32767ull != (short)3ull) || (((signed char)l0 <= (long long)g5) && ((long long)256ull < (unsigned char)1ull))) || ((signed char)l0 < (short)2ull))) && (((unsigned int)g0 > (unsigned char)ga1[0]) || ((long long)ga1[2] != (unsigned long long)32767ull))) && ((long long)65535ull <= (unsigned long long)32767ull))))))) >> ((unsigned)(unsigned int)(~(unsigned int)(signed char)f0((unsigned long long)g1, (int)ga0[2], (unsigned int)65535ull, (long long)g3)) & 31));
        }
    } else {
        p0 = (unsigned short)((unsigned int)(int)2ull - (unsigned int)(signed char)15ull);
    }
    return (signed char)ga0[0];
}

static unsigned short f3(int p0, short p1, short p2, signed char p3) {
    unsigned int l0 = (unsigned int)f1((unsigned int)(~(unsigned int)(int)3ull), (unsigned char)((unsigned int)(unsigned short)1ull | (unsigned int)(signed char)65535ull));
    unsigned int l1 = (unsigned int)((((unsigned int)0ull) || (!((unsigned int)g4 < (short)p1))) ? (long long)((unsigned long long)(unsigned int)87ull | (unsigned long long)(unsigned short)g5) : (unsigned char)((unsigned int)(unsigned long long)g1 >> ((unsigned)(unsigned int)32767ull & 31)));
    g5 = (unsigned char)(~(unsigned int)(long long)32767ull);
    p1 = (short)p1;
    if ((long long)((unsigned long long)(unsigned int)f2((unsigned short)g3) & (unsigned long long)(unsigned short)((unsigned int)(short)32697ull / ((unsigned int)(unsigned char)163ull | 1u))) > (unsigned int)((unsigned int)(unsigned int)((unsigned int)(int)ga1[4] ^ (unsigned int)(unsigned short)3ull) ^ (unsigned int)(unsigned long long)((unsigned long long)(long long)255ull % ((unsigned long long)(signed char)256ull | 1u)))) {
        if ((long long)((unsigned long long)(unsigned char)((unsigned int)(signed char)139ull / ((unsigned int)(short)1ull | 1u)) + (unsigned long long)(signed char)((unsigned int)(unsigned long long)1ull ^ (unsigned int)(unsigned long long)256ull)) >= (unsigned long long)((unsigned long long)(unsigned long long)(~(unsigned long long)(short)g6) | (unsigned long long)(int)((unsigned int)(short)7ull * (unsigned int)(short)0ull))) {
            g4 = (signed char)((unsigned int)(long long)ga1[4] >> ((unsigned)(unsigned int)((unsigned int)(long long)(((!((long long)g6 == (unsigned char)l0)) && ((int)255ull != (unsigned int)g2))) ^ (unsigned int)(unsigned long long)((unsigned long long)(unsigned short)g4 % ((unsigned long long)(long long)g0 | 1u))) & 31));
            p0 = (int)((unsigned int)(int)(((int)ga1[3] < (signed char)g2) ? (short)ga1[0] : (short)g4) << ((unsigned)(unsigned int)((unsigned int)(short)g6 * (unsigned int)(unsigned char)g1) & 31));
        } else {
            g4 = (signed char)f2((unsigned short)f1((unsigned int)((unsigned int)(int)((unsigned int)(short)255ull << ((unsigned)(unsigned int)1ull & 31)) + (unsigned int)(unsigned int)p2), (unsigned char)(((!((unsigned char)l1 > (unsigned long long)ga1[5])) || (((((int)ga1[4]) && ((int)256ull == (int)128ull)) || ((unsigned char)7ull < (int)ga0[6])) || ((unsigned short)p0 == (unsigned char)g6))) ? (int)f2((unsigned short)255ull) : (int)((unsigned int)(signed char)15ull << ((unsigned)(unsigned int)1ull & 31)))));
            g0 = (unsigned int)((unsigned int)(unsigned short)((unsigned int)(short)((unsigned int)0 - (unsigned int)(short)p2) / ((unsigned int)(short)((unsigned int)0 - (unsigned int)(signed char)p0) | 1u)) % ((unsigned int)(unsigned char)((((unsigned long long)p2 <= (unsigned short)256ull) && ((unsigned int)ga0[5] == (long long)ga1[0]))) | 1u));
        }
        if ((long long)(((unsigned char)g4 == (long long)ga0[2]) ? (unsigned short)(~(unsigned int)(unsigned int)ga0[6]) : (unsigned int)((unsigned int)(short)30155ull ^ (unsigned int)(signed char)255ull)) != (unsigned long long)((unsigned long long)(long long)127ull % ((unsigned long long)(unsigned char)((((unsigned int)l0 <= (unsigned int)1ull) && ((unsigned int)g6 != (short)127ull)) ? (short)p1 : (long long)65535ull) | 1u))) {
            g2 = (short)((unsigned int)(signed char)((unsigned int)(long long)f0((unsigned long long)p0, (int)p1, (unsigned int)128ull, (long long)g0) % ((unsigned int)(signed char)l1 | 1u)) * (unsigned int)(int)((unsigned int)(unsigned char)((unsigned int)(int)l1 >> ((unsigned)(unsigned int)ga0[3] & 31)) * (unsigned int)(long long)g3));
            g1 = (long long)ga1[2];
            g3 = (unsigned short)((unsigned int)(short)((!((((unsigned short)153ull <= (unsigned int)32767ull) && ((signed char)p1 < (short)p3)) && (((unsigned long long)7ull) || ((unsigned short)256ull == (long long)128ull))))) / ((unsigned int)(signed char)g5 | 1u));
        } else {
            p0 = (int)((unsigned int)0 - (unsigned int)(long long)p3);
        }
        switch ((int)((int)((unsigned int)(int)ga1[3] ^ (unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(unsigned char)2ull))) & 7) {
        case 0:
            g4 = (signed char)((unsigned int)(unsigned int)((unsigned int)(unsigned int)100ull % ((unsigned int)(unsigned char)g4 | 1u)) >> ((unsigned)(unsigned int)(((short)ga1[2] <= (unsigned int)65535ull) ? (int)2ull : (signed char)p3) & 31));
            break;
        case 4:
            p0 = (int)((unsigned int)(int)0ull + (unsigned int)(short)12534ull);
        case 5:
            p1 = (short)(~(unsigned int)(unsigned int)((unsigned int)(long long)((unsigned long long)(unsigned char)(((signed char)7ull <= (long long)ga0[2])) % ((unsigned long long)(short)((unsigned int)0 - (unsigned int)(unsigned int)ga0[3]) | 1u)) * (unsigned int)(int)((unsigned int)(unsigned char)(~(unsigned int)(unsigned long long)l0) << ((unsigned)(unsigned int)((unsigned int)(int)ga1[0] | (unsigned int)(long long)1ull) & 31))));
        default:
            p2 = (short)((((unsigned char)l1 < (int)g0) && ((((short)p3 >= (long long)3ull) || ((unsigned short)3ull <= (unsigned short)p2)) && (((unsigned short)0ull >= (unsigned short)2ull) || ((long long)l1 < (int)255ull)))) ? (unsigned char)((unsigned int)(short)ga1[5] | (unsigned int)(unsigned long long)g2) : (unsigned int)7ull);
        }
    }
    for (int i0 = 0; i0 < 5; i0++) {
        { int w1 = 3; while (w1 > 0) {
            g2 = (short)p0;
            w1--;
        } }
    }
    return (unsigned short)((unsigned int)(signed char)g5 % ((unsigned int)(int)((unsigned int)(int)g4 * (unsigned int)(unsigned short)15ull) | 1u));
}

int main(void) {
    int m0 = (int)0ull;
    signed char m1 = (signed char)15ull;
    unsigned long long m2 = (unsigned long long)1ull;
    unsigned int m3 = (unsigned int)2ull;
    g0 = (unsigned int)g6;
    m1 = (signed char)((unsigned int)(short)128ull % ((unsigned int)(short)((((long long)g4 < (short)g6) && (((unsigned short)2ull) || ((((long long)g1) || (((signed char)ga0[4] <= (unsigned short)255ull) && ((((((((unsigned short)g5) || ((unsigned short)ga1[2] < (unsigned int)3ull)) && ((short)ga1[2] != (unsigned short)139ull)) || (((long long)m1 >= (unsigned short)1ull) || (((short)ga1[2]) || ((unsigned short)g0 < (long long)m2)))) && (((unsigned long long)ga1[2] == (unsigned long long)g0) && (((unsigned int)g2 < (long long)1ull) || (((unsigned int)m2 < (unsigned short)m0) && ((int)ga1[0] == (int)ga0[2]))))) && ((short)100ull >= (unsigned char)ga0[3])) || ((((long long)11358735226509898056ull <= (signed char)15ull) || (((int)2ull > (unsigned int)g0) || ((short)g6 <= (unsigned char)ga0[4]))) || (!(((((int)ga0[4] < (short)0ull) && ((signed char)128ull > (int)100ull)) && ((signed char)ga1[5] >= (short)g2)) || (!(!((int)g6 > (unsigned char)g2))))))))) && ((unsigned int)ga0[2] < (int)1ull))))) | 1u));
    m2 = (unsigned long long)((unsigned long long)(int)15ull | (unsigned long long)(unsigned long long)m2);
    ga0[6] = (unsigned short)((((((unsigned char)g6) && ((short)ga0[4] > (signed char)g3)) || (((((long long)m0 == (signed char)128ull) && (((unsigned int)ga1[5] >= (long long)m2) && ((int)3ull != (unsigned short)15ull))) && ((unsigned char)ga1[2] != (short)m3)) && (!((unsigned int)ga1[5] == (unsigned char)255ull)))) && ((unsigned long long)((unsigned long long)(signed char)211ull / ((unsigned long long)(unsigned short)1ull | 1u)) == (int)f1((unsigned int)256ull, (unsigned char)m2))) ? (unsigned long long)((unsigned long long)(short)m1 - (unsigned long long)(signed char)((unsigned int)(unsigned int)g6 >> ((unsigned)(unsigned int)100ull & 31))) : (unsigned int)ga1[2]);
    g6 = (short)f3((int)(((unsigned char)256ull < (unsigned char)g1)), (short)ga0[5], (short)((unsigned int)(signed char)g0 << ((unsigned)(unsigned int)3ull & 31)), (signed char)((!(((unsigned short)g2 <= (long long)0ull) && ((signed char)32767ull < (unsigned char)ga0[5]))) ? (unsigned int)7ull : (unsigned long long)g0));
    g5 = (unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(unsigned short)(((unsigned long long)g1) ? (unsigned int)m2 : (long long)m2) - (unsigned long long)(unsigned int)256ull) >> ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(unsigned int)1ull) & 31));
    switch ((int)((int)g5) & 7) {
    case 1:
        switch ((int)((int)((((unsigned short)g5 <= (int)87ull) && ((((((unsigned int)ga0[2] >= (short)0ull) || ((!((unsigned long long)ga0[5] > (unsigned short)ga1[5])) || ((signed char)128ull))) || ((signed char)g2 != (long long)m2)) && ((short)ga0[1] != (signed char)127ull)) || ((((short)65535ull == (long long)ga0[6]) || ((unsigned char)32767ull >= (unsigned short)3ull)) || ((int)m0 < (short)g3)))) ? (unsigned short)((unsigned int)(unsigned char)g0 ^ (unsigned int)(unsigned short)15ull) : (int)((unsigned int)0 - (unsigned int)(long long)m0))) & 7) {
        case 0:
            if ((unsigned char)f2((unsigned short)f2((unsigned short)g3))) {
                m1 = (signed char)(~(unsigned int)(short)ga1[2]);
                g4 = (signed char)((unsigned int)(int)((unsigned int)0 - (unsigned int)(signed char)g1) - (unsigned int)(unsigned char)f0((unsigned long long)m2, (int)15ull, (unsigned int)g2, (long long)ga0[4]));
                g4 = (signed char)((unsigned int)(int)f1((unsigned int)((unsigned int)(unsigned char)252ull ^ (unsigned int)(short)(((short)255ull >= (int)g0))), (unsigned char)((unsigned int)0 - (unsigned int)(unsigned short)((unsigned int)(unsigned short)g4 >> ((unsigned)(unsigned int)2ull & 31)))) & (unsigned int)(long long)f3((int)((unsigned int)(unsigned int)((unsigned int)(unsigned int)m1 & (unsigned int)(unsigned long long)127ull) >> ((unsigned)(unsigned int)((unsigned int)(signed char)ga0[1] | (unsigned int)(short)101ull) & 31)), (short)((!((unsigned long long)m1 < (signed char)0ull)) ? (unsigned short)(~(unsigned int)(long long)ga0[0]) : (long long)((unsigned long long)(unsigned int)ga0[1] - (unsigned long long)(unsigned int)ga0[0])), (short)255ull, (signed char)((unsigned int)0 - (unsigned int)(short)((unsigned int)(unsigned int)g3 + (unsigned int)(int)g5))));
            }
            break;
        case 3:
            g6 = (short)32767ull;
            break;
        case 7:
            g6 = (short)((unsigned int)(unsigned long long)((unsigned long long)(signed char)((unsigned int)(short)((((int)65535ull != (signed char)127ull) && ((unsigned short)m3 > (int)g0)) ? (signed char)ga0[0] : (short)127ull) << ((unsigned)(unsigned int)((unsigned int)(unsigned int)1ull - (unsigned int)(unsigned long long)g2) & 31)) | (unsigned long long)(unsigned short)((unsigned int)(unsigned long long)(~(unsigned long long)(unsigned short)3ull) << ((unsigned)(unsigned int)((unsigned int)(unsigned long long)ga0[2] << ((unsigned)(unsigned int)15ull & 31)) & 31))) ^ (unsigned int)(unsigned long long)((unsigned long long)(long long)(((int)((unsigned int)(unsigned long long)7ull & (unsigned int)(short)15ull) <= (unsigned short)(((signed char)g2 < (int)m3)))) / ((unsigned long long)(unsigned int)((unsigned int)0 - (unsigned int)(signed char)((unsigned int)(unsigned short)m2 * (unsigned int)(unsigned long long)g0)) | 1u)));
            break;
        default:
            m3 = (unsigned int)(~(unsigned int)(signed char)((unsigned int)(int)0ull + (unsigned int)(unsigned short)m0));
        }
    case 5:
        m2 = (unsigned long long)g0;
        break;
    case 7:
        switch ((int)((int)((unsigned int)(unsigned char)g3 & (unsigned int)(unsigned char)f0((unsigned long long)32767ull, (int)m2, (unsigned int)256ull, (long long)g3))) & 7) {
        case 1:
            if (((signed char)((unsigned int)(unsigned int)ga0[5] % ((unsigned int)(unsigned short)ga0[6] | 1u)) >= (signed char)g4) || ((signed char)((unsigned int)(long long)19ull >> ((unsigned)(unsigned int)167ull & 31)) >= (unsigned long long)((unsigned long long)(unsigned char)256ull ^ (unsigned long long)(unsigned long long)14830703688720288588ull))) {
                g0 = (unsigned int)g4;
            }
            break;
        case 3:
            m0 = (int)((unsigned int)(unsigned int)((unsigned int)(unsigned int)256ull ^ (unsigned int)(unsigned short)((unsigned int)(int)256ull % ((unsigned int)(short)15ull | 1u))) / ((unsigned int)(signed char)g6 | 1u));
            break;
        case 6:
            g1 = (long long)(~(unsigned long long)(long long)((unsigned long long)(unsigned short)ga1[0] / ((unsigned long long)(signed char)ga0[2] | 1u)));
        case 7:
            m1 = (signed char)ga0[5];
        default:
            g5 = (unsigned char)(~(unsigned int)(unsigned int)f1((unsigned int)((unsigned int)(long long)2ull | (unsigned int)(unsigned long long)ga1[4]), (unsigned char)((((signed char)255ull == (int)m2) && (((unsigned char)m2 == (short)m3) && ((((unsigned char)m1 > (unsigned long long)g4) && ((unsigned char)32767ull != (unsigned long long)g4)) || (((long long)m3 > (signed char)g0) || (((unsigned long long)0ull != (short)256ull) && ((long long)255ull >= (signed char)g4)))))) ? (long long)100ull : (unsigned int)ga1[1])));
        }
        break;
    default:
        if ((((int)32767ull) || (((long long)15ull != (unsigned long long)g0) || ((unsigned int)ga1[1] >= (signed char)ga1[3]))) || (((unsigned int)ga0[1]) && ((unsigned int)ga0[5] > (long long)3ull))) {
            ga1[1] = (unsigned short)(((unsigned int)m1 >= (unsigned char)((unsigned int)(unsigned short)f2((unsigned short)g1) * (unsigned int)(unsigned char)7ull)) ? (int)g0 : (unsigned long long)((unsigned long long)(unsigned long long)m3 - (unsigned long long)(signed char)1ull));
            g6 = (short)m1;
        } else {
            if (((int)(~(unsigned int)(short)256ull) < (unsigned int)((unsigned int)0 - (unsigned int)(unsigned char)ga0[0])) || (((signed char)g6 >= (int)m2) || (((short)15ull > (int)g0) || ((unsigned char)100ull > (unsigned long long)g3)))) {
                ga0[5] = (unsigned short)g1;
            } else {
                m3 = (unsigned int)32767ull;
                g4 = (signed char)m3;
                ga1[3] = (unsigned short)((unsigned int)(unsigned short)(((signed char)((unsigned int)(unsigned char)g5 * (unsigned int)(short)3ull) > (unsigned long long)((unsigned long long)(signed char)g3 | (unsigned long long)(short)ga0[4])) ? (unsigned char)127ull : (unsigned int)((unsigned int)0 - (unsigned int)(unsigned char)ga1[5])) + (unsigned int)(unsigned char)g5);
            }
        }
    }
    m0 = (int)f1((unsigned int)(((short)f2((unsigned short)g2)) ? (unsigned char)((unsigned int)(signed char)255ull + (unsigned int)(long long)128ull) : (unsigned char)((unsigned int)(short)m1 ^ (unsigned int)(unsigned int)3333576622ull)), (unsigned char)128ull);
    g6 = (short)(~(unsigned int)(long long)((unsigned long long)(unsigned char)((unsigned int)(unsigned long long)g2 * (unsigned int)(long long)128ull) % ((unsigned long long)(int)((unsigned int)(unsigned char)128ull / ((unsigned int)(short)ga0[5] | 1u)) | 1u)));
    if ((long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned char)3ull * (unsigned int)(long long)2ull) / ((unsigned long long)(int)m3 | 1u)) < (unsigned short)f3((int)((unsigned int)(int)331300647ull ^ (unsigned int)(unsigned char)32767ull), (short)((unsigned int)(signed char)244ull | (unsigned int)(unsigned int)ga1[4]), (short)(((unsigned short)143ull != (int)15ull)), (signed char)g0)) {
        switch ((int)((int)m2) & 7) {
        case 0:
            m2 = (unsigned long long)(((unsigned char)((unsigned int)(short)((unsigned int)(signed char)((unsigned int)(unsigned int)256ull - (unsigned int)(unsigned short)ga0[6]) ^ (unsigned int)(unsigned int)((unsigned int)(unsigned char)128ull >> ((unsigned)(unsigned int)g4 & 31))) + (unsigned int)(unsigned long long)g0) == (short)((unsigned int)(short)((unsigned int)(signed char)(~(unsigned int)(long long)128ull) + (unsigned int)(unsigned char)((unsigned int)(int)ga0[4] >> ((unsigned)(unsigned int)0ull & 31))) / ((unsigned int)(unsigned long long)g5 | 1u))) ? (unsigned short)((!(((int)ga1[2] <= (signed char)g5) && ((long long)15ull <= (short)g1)))) : (signed char)32767ull);
            break;
        case 2:
            m0 = (int)2ull;
            break;
        case 5:
            m1 = (signed char)(((unsigned int)((unsigned int)(unsigned int)g0 + (unsigned int)(int)ga0[2]) > (unsigned long long)((unsigned long long)(unsigned short)m1 - (unsigned long long)(unsigned int)g0)) ? (unsigned short)(((signed char)100ull < (signed char)65535ull) ? (long long)g3 : (unsigned short)15ull) : (signed char)g1);
            break;
        case 6:
            ga1[5] = (unsigned short)f2((unsigned short)g5);
            break;
        default:
            ga1[2] = (unsigned short)((unsigned int)0 - (unsigned int)(signed char)((((unsigned char)g1 > (signed char)255ull) || (((unsigned int)127ull > (unsigned char)100ull) && ((signed char)0ull))) ? (unsigned long long)((unsigned long long)(long long)g1 | (unsigned long long)(unsigned char)ga0[2]) : (signed char)((unsigned int)(signed char)2ull / ((unsigned int)(int)m3 | 1u))));
        }
    } else {
        { int w0 = 4; while (w0 > 0) {
            m3 = (unsigned int)m0;
            w0--;
        } }
        g5 = (unsigned char)((unsigned int)(unsigned char)g5 * (unsigned int)(int)((unsigned int)(short)g0 | (unsigned int)(long long)f3((int)(((unsigned char)ga1[1] >= (unsigned char)g1) ? (signed char)7ull : (long long)g3), (short)((unsigned int)(signed char)256ull / ((unsigned int)(short)3ull | 1u)), (short)((unsigned int)(short)ga0[2] / ((unsigned int)(short)m3 | 1u)), (signed char)((unsigned int)(unsigned char)ga1[3] * (unsigned int)(unsigned short)ga1[3]))));
    }
    if ((long long)f2((unsigned short)((unsigned int)(unsigned int)g5 + (unsigned int)(unsigned long long)m1)) <= (short)ga1[5]) {
        switch ((int)((int)(((unsigned int)((unsigned int)(short)ga0[2] >> ((unsigned)(unsigned int)g3 & 31))))) & 7) {
        case 4:
            switch ((int)((int)((unsigned int)(unsigned int)((unsigned int)(int)ga0[2] / ((unsigned int)(unsigned short)m0 | 1u)) >> ((unsigned)(unsigned int)f1((unsigned int)ga1[1], (unsigned char)m2) & 31))) & 7) {
            case 1:
                g3 = (unsigned short)((unsigned int)(short)((unsigned int)(int)((unsigned int)(int)m3 | (unsigned int)(unsigned char)ga1[1]) << ((unsigned)(unsigned int)2ull & 31)) + (unsigned int)(unsigned short)f1((unsigned int)f2((unsigned short)ga1[0]), (unsigned char)f2((unsigned short)g4)));
                break;
            case 5:
                g4 = (signed char)((unsigned int)(signed char)255ull | (unsigned int)(short)ga1[1]);
                break;
            case 6:
                g6 = (short)((unsigned int)(signed char)255ull | (unsigned int)(unsigned long long)ga1[4]);
                break;
            case 7:
                g3 = (unsigned short)(((unsigned int)g0 > (signed char)g0) ? (short)g4 : (unsigned long long)ga0[5]);
                break;
            default:
                ga0[5] = (unsigned short)255ull;
            }
            break;
        case 7:
            g3 = (unsigned short)((unsigned int)(signed char)((unsigned int)(unsigned char)f2((unsigned short)(((((unsigned int)g1 != (signed char)ga1[2]) && ((((unsigned short)50827ull < (unsigned char)g2) || (((signed char)ga1[0] > (unsigned short)65535ull) || (((signed char)32767ull <= (unsigned long long)g4) || ((short)25623ull < (signed char)29ull)))) && ((unsigned int)2ull <= (long long)ga0[2]))) || ((((short)128ull == (int)3ull) && (!((((unsigned int)1ull <= (short)ga1[0]) && ((unsigned char)ga1[1] == (unsigned long long)g3)) && ((signed char)g5 > (unsigned short)g0)))) && (((long long)ga1[2] < (unsigned long long)ga1[4]) && (((signed char)g0 != (long long)256ull) || (((unsigned int)g3 <= (long long)g2) && (((long long)m3 >= (unsigned int)255ull) && ((long long)m0 < (unsigned int)m3))))))) ? (unsigned short)65535ull : (int)m2)) - (unsigned int)(int)((unsigned int)(unsigned long long)((unsigned long long)(short)2ull | (unsigned long long)(unsigned char)m1) >> ((unsigned)(unsigned int)((unsigned int)(unsigned int)g1 | (unsigned int)(unsigned long long)g1) & 31))) + (unsigned int)(long long)((unsigned long long)(short)((unsigned int)(unsigned short)(~(unsigned int)(unsigned long long)2ull) & (unsigned int)(unsigned short)((unsigned int)(signed char)ga1[4] % ((unsigned int)(unsigned long long)g5 | 1u))) - (unsigned long long)(unsigned int)((unsigned int)(short)((unsigned int)(unsigned long long)100ull * (unsigned int)(unsigned long long)g4) + (unsigned int)(unsigned char)m0)));
            break;
        default:
            switch ((int)((int)((unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(short)ga1[3]) % ((unsigned int)(signed char)((((unsigned long long)3ull <= (short)ga0[6]) || ((unsigned long long)g3 >= (unsigned char)ga0[3])) ? (unsigned long long)ga0[0] : (unsigned int)1ull) | 1u))) & 7) {
            case 1:
                ga0[1] = (unsigned short)((unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(int)m2) + (unsigned int)(unsigned short)(((short)((unsigned int)(unsigned int)3ull + (unsigned int)(short)m1) >= (signed char)g1) ? (short)m2 : (unsigned char)((unsigned int)(unsigned int)g5 << ((unsigned)(unsigned int)127ull & 31))));
                break;
            case 6:
                m2 = (unsigned long long)((unsigned long long)(short)(~(unsigned int)(unsigned short)100ull) + (unsigned long long)(unsigned long long)((unsigned long long)(short)g4 >> ((unsigned)(unsigned int)g0 & 63)));
            case 7:
                m3 = (unsigned int)((unsigned int)(int)((((unsigned char)256ull < (unsigned char)g2) && (!((short)g0 <= (unsigned char)ga0[2])))) + (unsigned int)(signed char)((unsigned int)(unsigned short)127ull << ((unsigned)(unsigned int)g0 & 31)));
                break;
            default:
                m2 = (unsigned long long)((unsigned long long)(int)((unsigned int)(int)((unsigned int)(long long)1ull * (unsigned int)(signed char)128ull) * (unsigned int)(unsigned long long)((unsigned long long)(unsigned char)100ull << ((unsigned)(unsigned int)ga0[3] & 63))) + (unsigned long long)(int)7ull);
            }
        }
    } else {
        if ((unsigned char)m3 <= (unsigned long long)((unsigned long long)(long long)0ull / ((unsigned long long)(unsigned int)((unsigned int)0 - (unsigned int)(int)15ull) | 1u))) {
            for (int i1 = 0; i1 < 1; i1++) {
                ga1[5] = (unsigned short)(((long long)((unsigned long long)(unsigned short)((unsigned int)(signed char)ga1[3] >> ((unsigned)(unsigned int)255ull & 31)) & (unsigned long long)(unsigned int)((unsigned int)(unsigned int)g5 % ((unsigned int)(unsigned short)15ull | 1u))) != (short)((unsigned int)(unsigned long long)(((int)g3)) + (unsigned int)(unsigned short)((unsigned int)(unsigned char)7ull << ((unsigned)(unsigned int)ga1[5] & 31)))) ? (short)((unsigned int)(signed char)((unsigned int)0 - (unsigned int)(long long)100ull) + (unsigned int)(int)((unsigned int)(unsigned char)m3 % ((unsigned int)(unsigned short)g6 | 1u))) : (long long)((unsigned long long)(int)7ull * (unsigned long long)(unsigned long long)((unsigned long long)(signed char)256ull / ((unsigned long long)(short)127ull | 1u))));
                m3 = (unsigned int)m3;
            }
            g1 = (long long)((unsigned long long)(unsigned char)g4 / ((unsigned long long)(unsigned long long)128ull | 1u));
        }
        g3 = (unsigned short)127ull;
        g0 = (unsigned int)f0((unsigned long long)g3, (int)2ull, (unsigned int)127ull, (long long)255ull);
    }
    { int w2 = 1; while (w2 > 0) {
        { int w3 = 5; while (w3 > 0) {
            if ((unsigned int)((unsigned int)(unsigned long long)255ull * (unsigned int)(unsigned char)m1) == (unsigned char)((unsigned int)(short)((unsigned int)(unsigned int)ga0[6] - (unsigned int)(unsigned int)ga0[5]) * (unsigned int)(unsigned int)((unsigned int)(short)0ull - (unsigned int)(unsigned long long)g3))) {
                g1 = (long long)((unsigned long long)(int)((unsigned int)(unsigned short)((unsigned int)(unsigned char)ga0[5] * (unsigned int)(unsigned long long)w3) - (unsigned int)(int)(~(unsigned int)(unsigned long long)ga1[3])) | (unsigned long long)(unsigned short)((unsigned int)(signed char)m1 & (unsigned int)(unsigned int)((unsigned int)(unsigned short)ga1[2] ^ (unsigned int)(unsigned short)ga1[3])));
            }
            w3--;
        } }
        w2--;
    } }
    for (int i4 = 0; i4 < 5; i4++) {
        { int w5 = 2; while (w5 > 0) {
            m0 = (int)ga1[4];
            w5--;
        } }
        for (int i6 = 0; i6 < 5; i6++) {
            for (int i7 = 0; i7 < 5; i7++) {
                g6 = (short)((unsigned int)0 - (unsigned int)(unsigned int)2ull);
                ga0[2] = (unsigned short)(~(unsigned int)(unsigned int)i4);
            }
            g0 = (unsigned int)ga1[0];
            g6 = (short)f1((unsigned int)g4, (unsigned char)g6);
        }
        for (int i8 = 0; i8 < 1; i8++) {
            m3 = (unsigned int)m1;
            if ((int)((unsigned int)(int)65535ull - (unsigned int)(unsigned char)((unsigned int)(unsigned char)2ull * (unsigned int)(unsigned int)g5)) == (int)(((unsigned int)((unsigned int)(short)ga1[1] * (unsigned int)(unsigned long long)ga1[1]) == (unsigned long long)((unsigned long long)(signed char)151ull - (unsigned long long)(unsigned long long)128ull)))) {
                ga0[5] = (unsigned short)((unsigned int)(unsigned char)((!(((unsigned short)g6 < (unsigned char)0ull) || ((unsigned short)100ull <= (int)ga0[4]))) ? (short)g1 : (signed char)((unsigned int)(unsigned long long)m1 & (unsigned int)(unsigned short)g3)) | (unsigned int)(long long)((((signed char)g0 < (signed char)ga0[6]) && (((unsigned char)ga0[4] >= (unsigned short)ga1[3]) || ((((((signed char)7ull >= (unsigned short)ga0[4]) || ((unsigned char)100ull == (short)12358ull)) || ((int)ga1[5])) || (((unsigned char)g3 == (unsigned int)g0) && ((unsigned char)m0 < (unsigned long long)ga0[0]))) && ((int)65535ull >= (unsigned short)m0))))));
            }
        }
    }
    PR("g0", g0);
    PR("g1", g1);
    PR("g2", g2);
    PR("g3", g3);
    PR("g4", g4);
    PR("g5", g5);
    PR("g6", g6);
    PR("m0", m0);
    PR("m1", m1);
    PR("m2", m2);
    PR("m3", m3);
    PR("ga0_0", ga0[0]);
    PR("ga0_1", ga0[1]);
    PR("ga0_2", ga0[2]);
    PR("ga0_3", ga0[3]);
    PR("ga0_4", ga0[4]);
    PR("ga0_5", ga0[5]);
    PR("ga0_6", ga0[6]);
    PR("ga1_0", ga1[0]);
    PR("ga1_1", ga1[1]);
    PR("ga1_2", ga1[2]);
    PR("ga1_3", ga1[3]);
    PR("ga1_4", ga1[4]);
    PR("ga1_5", ga1[5]);
    return 0;
}

