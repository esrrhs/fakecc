#include <stdio.h>
#define PR(n, v) printf(n "=%llx\n", (unsigned long long)(v))
long long g0 = (long long)100ull;
unsigned long long g1 = (unsigned long long)128ull;
signed char g2 = (signed char)127ull;
unsigned long long g3 = (unsigned long long)2ull;
unsigned int g4 = (unsigned int)2ull;
unsigned char g5 = (unsigned char)255ull;
unsigned int g6 = (unsigned int)127ull;
unsigned char g7 = (unsigned char)255ull;
int g8 = (int)1ull;
int ga0[6] = {(int)15ull, (int)1ull, (int)1ull, (int)15ull, (int)1ull, (int)127ull};
unsigned int ga1[7] = {(unsigned int)255ull, (unsigned int)2830432041ull, (unsigned int)256ull, (unsigned int)0ull, (unsigned int)7ull, (unsigned int)127ull, (unsigned int)128ull};
static unsigned int f0(signed char p0, int p1, unsigned int p2, short p3) {
    long long l0 = (long long)(~(unsigned long long)(long long)g8);
    unsigned long long l1 = (unsigned long long)g1;
    g8 = (int)(((unsigned char)((!((int)((unsigned int)(short)p3 / ((unsigned int)(unsigned short)ga1[3] | 1u)) > (unsigned long long)g6)) ? (unsigned int)((unsigned int)(short)((unsigned int)(unsigned long long)g6 & (unsigned int)(short)g1) - (unsigned int)(unsigned int)((unsigned int)(signed char)ga1[5] - (unsigned int)(unsigned long long)2ull)) : (unsigned int)((unsigned int)(long long)((unsigned long long)(long long)ga0[2] + (unsigned long long)(short)l0) - (unsigned int)(signed char)((unsigned int)(unsigned int)g5 >> ((unsigned)(unsigned int)g2 & 31)))) < (signed char)((unsigned int)(short)l0 - (unsigned int)(long long)g7)) ? (long long)((unsigned long long)(unsigned char)l0 ^ (unsigned long long)(short)((unsigned int)0 - (unsigned int)(int)(((long long)ga0[1] < (signed char)2ull) ? (unsigned char)ga0[2] : (unsigned char)7ull))) : (unsigned char)p3);
    switch ((int)((int)972688959ull) & 7) {
    case 0:
        p1 = (int)(~(unsigned int)(short)((((short)0ull < (int)3ull) && ((unsigned int)g4))));
    case 1:
        if (((!(!((short)1ull >= (unsigned short)92ull))) || (((((unsigned char)p3) && (((short)l1 != (long long)0ull) && (((int)15ull > (short)g7) || ((unsigned int)7ull < (long long)2ull)))) && ((unsigned int)g5 <= (long long)ga0[2])) || ((unsigned char)p3 == (unsigned short)100ull))) && ((unsigned char)((unsigned int)(int)255ull / ((unsigned int)(unsigned short)65535ull | 1u)) != (unsigned short)((unsigned int)(short)l0 >> ((unsigned)(unsigned int)255ull & 31)))) {
            l1 = (unsigned long long)p0;
        } else {
            p3 = (short)((unsigned int)(long long)((unsigned long long)(unsigned char)(~(unsigned int)(unsigned char)127ull) * (unsigned long long)(signed char)g1) + (unsigned int)(unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(unsigned int)127ull & (unsigned long long)(signed char)p1) - (unsigned int)(int)ga1[1]));
            g8 = (int)((unsigned int)(unsigned short)ga0[0] | (unsigned int)(unsigned short)ga0[3]);
            p2 = (unsigned int)((unsigned int)(signed char)((unsigned int)(unsigned int)((unsigned int)(unsigned long long)p3 * (unsigned int)(unsigned int)p1) >> ((unsigned)(unsigned int)32767ull & 31)) ^ (unsigned int)(signed char)((unsigned int)(unsigned char)((unsigned int)(short)ga1[3] - (unsigned int)(unsigned short)ga1[4]) & (unsigned int)(unsigned int)((unsigned int)(int)1ull + (unsigned int)(unsigned int)1ull)));
        }
        break;
    case 4:
        g2 = (signed char)((unsigned int)(long long)0ull + (unsigned int)(long long)((unsigned long long)(signed char)((unsigned int)(signed char)3ull >> ((unsigned)(unsigned int)32767ull & 31)) + (unsigned long long)(int)((unsigned int)(short)ga1[5] + (unsigned int)(unsigned short)p0)));
        break;
    case 7:
        g2 = (signed char)g3;
    default:
        g3 = (unsigned long long)((unsigned long long)(short)ga0[3] & (unsigned long long)(unsigned int)28ull);
    }
    p0 = (signed char)(((unsigned int)l0 > (int)l1) ? (int)((unsigned int)(unsigned short)(~(unsigned int)(unsigned char)(~(unsigned int)(long long)p3)) / ((unsigned int)(unsigned char)p2 | 1u)) : (short)(((signed char)p2 == (short)((unsigned int)(signed char)((unsigned int)(int)100ull << ((unsigned)(unsigned int)7ull & 31)) - (unsigned int)(unsigned int)((unsigned int)(unsigned int)g7 / ((unsigned int)(unsigned int)65535ull | 1u))))));
    g8 = (int)((unsigned int)(int)((unsigned int)(int)((unsigned int)(long long)ga0[2] + (unsigned int)(short)32767ull) | (unsigned int)(int)(~(unsigned int)(short)g7)) * (unsigned int)(unsigned long long)(~(unsigned long long)(unsigned short)(~(unsigned int)(unsigned short)p0)));
    return (unsigned int)((unsigned int)(unsigned short)((unsigned int)(unsigned int)3ull ^ (unsigned int)(int)ga1[4]) << ((unsigned)(unsigned int)((unsigned int)(signed char)p0 % ((unsigned int)(short)45ull | 1u)) & 31));
}

static unsigned long long f1(short p0, unsigned char p1, unsigned int p2) {
    long long l0 = (long long)((unsigned long long)(unsigned char)(~(unsigned int)(signed char)65535ull) & (unsigned long long)(long long)((unsigned long long)0 - (unsigned long long)(unsigned long long)p1));
    for (int i0 = 0; i0 < 5; i0++) {
        if ((unsigned short)3ull <= (unsigned char)p2) {
            ga1[0] = (unsigned int)((unsigned int)(unsigned long long)(~(unsigned long long)(unsigned long long)((unsigned long long)(unsigned char)90ull & (unsigned long long)(unsigned long long)1ull)) >> ((unsigned)(unsigned int)((unsigned int)(unsigned short)ga1[4] % ((unsigned int)(long long)((unsigned long long)(int)127ull % ((unsigned long long)(unsigned char)p2 | 1u)) | 1u)) & 31));
        }
    }
    { int w1 = 5; while (w1 > 0) {
        if (((unsigned char)l0 == (unsigned short)(((((unsigned long long)32767ull <= (unsigned long long)1ull) || ((short)g4 > (short)g7)) && ((short)1ull != (signed char)g7)) ? (short)g8 : (signed char)g8)) && ((unsigned long long)((unsigned long long)(unsigned char)0ull >> ((unsigned)(unsigned int)134ull & 63)))) {
            p2 = (unsigned int)g8;
            g3 = (unsigned long long)((unsigned long long)(long long)((unsigned long long)(long long)g6 / ((unsigned long long)(unsigned short)100ull | 1u)) + (unsigned long long)(unsigned short)32767ull);
            g4 = (unsigned int)((unsigned int)(signed char)ga1[6] | (unsigned int)(long long)1ull);
        } else {
            g0 = (long long)((unsigned long long)(unsigned short)((unsigned int)(long long)g1 - (unsigned int)(short)8072ull) >> ((unsigned)(unsigned int)((!((!(!((unsigned short)ga1[0] == (unsigned short)32767ull))) && ((unsigned int)ga0[2])))) & 63));
        }
        w1--;
    } }
    switch ((int)((int)((unsigned int)(signed char)((unsigned int)0 - (unsigned int)(signed char)0ull) ^ (unsigned int)(long long)g8)) & 7) {
    case 0:
        switch ((int)((int)((((((unsigned int)15ull >= (unsigned char)g0) || ((long long)ga0[5] == (unsigned int)g0)) && ((unsigned int)ga1[1] >= (unsigned long long)7ull)) || ((unsigned long long)g3 == (unsigned long long)g2)))) & 7) {
        case 1:
            g1 = (unsigned long long)((((signed char)((unsigned int)(unsigned char)ga1[1] / ((unsigned int)(unsigned char)g5 | 1u)) > (unsigned short)((unsigned int)(long long)256ull & (unsigned int)(signed char)255ull)) && (((unsigned int)202ull) && (((int)g3 >= (unsigned long long)15ull) && ((int)g7 < (short)g2)))));
            break;
        case 5:
            ga1[0] = (unsigned int)((unsigned int)(long long)15ull * (unsigned int)(signed char)((unsigned int)(unsigned short)((unsigned int)(long long)ga1[3] << ((unsigned)(unsigned int)29ull & 31)) >> ((unsigned)(unsigned int)(((long long)g4 <= (unsigned int)15ull) ? (unsigned char)g5 : (unsigned int)ga0[0]) & 31)));
            break;
        case 7:
            g6 = (unsigned int)255ull;
        default:
            g1 = (unsigned long long)((unsigned long long)(long long)((unsigned long long)0 - (unsigned long long)(unsigned long long)p0) * (unsigned long long)(signed char)((((((unsigned char)65535ull != (int)2465951395ull) || (((unsigned short)ga1[0] >= (short)l0) && ((unsigned int)ga1[5] != (long long)g2))) || (!((int)p0 != (unsigned long long)124ull))) && ((long long)((unsigned long long)(unsigned char)ga1[4] << ((unsigned)(unsigned int)g0 & 63)) != (unsigned char)((unsigned int)(unsigned long long)g8 + (unsigned int)(unsigned long long)ga0[2])))));
        }
        break;
    case 1:
        ga0[0] = (int)g8;
        break;
    case 7:
        g3 = (unsigned long long)((unsigned long long)(long long)(((unsigned long long)p0) ? (unsigned int)((unsigned int)(unsigned char)((unsigned int)(unsigned long long)100ull << ((unsigned)(unsigned int)l0 & 31)) - (unsigned int)(long long)((unsigned long long)(signed char)ga1[6] / ((unsigned long long)(long long)p1 | 1u))) : (long long)(((signed char)f0((signed char)p2, (int)g7, (unsigned int)256ull, (short)g1) != (unsigned long long)((unsigned long long)(unsigned short)ga0[0] << ((unsigned)(unsigned int)100ull & 63))))) | (unsigned long long)(signed char)((unsigned int)(int)f0((signed char)((unsigned int)(unsigned short)ga1[1] / ((unsigned int)(unsigned long long)p2 | 1u)), (int)(~(unsigned int)(short)2ull), (unsigned int)(((((int)1153077770ull >= (unsigned int)g0) && ((unsigned char)g8 != (short)ga0[0])) && ((int)100ull != (unsigned long long)ga1[4])) ? (long long)65535ull : (unsigned long long)g0), (short)((unsigned int)(int)p0 ^ (unsigned int)(unsigned long long)p2)) ^ (unsigned int)(unsigned long long)32767ull));
        break;
    default:
        g2 = (signed char)p0;
    }
    g3 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(unsigned short)15ull | (unsigned long long)(int)65535ull) * (unsigned long long)(unsigned long long)(((unsigned long long)ga0[2] == (int)3ull) ? (int)100ull : (short)g4));
    return (unsigned long long)((unsigned long long)(int)((unsigned int)(signed char)p2 >> ((unsigned)(unsigned int)ga0[4] & 31)) * (unsigned long long)(unsigned short)((unsigned int)(unsigned long long)256ull & (unsigned int)(unsigned int)ga1[4]));
}

int main(void) {
    unsigned int m0 = (unsigned int)7ull;
    unsigned int m1 = (unsigned int)3ull;
    signed char m2 = (signed char)217ull;
    g3 = (unsigned long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned char)3ull - (unsigned int)(signed char)g4) + (unsigned long long)(int)((unsigned int)0 - (unsigned int)(int)m2));
    m2 = (signed char)((unsigned int)(short)((unsigned int)(unsigned int)g0 - (unsigned int)(unsigned char)(((signed char)(((unsigned int)54ull > (short)g7) ? (signed char)255ull : (unsigned short)g2) > (short)f1((short)g8, (unsigned char)ga0[2], (unsigned int)2ull)) ? (long long)((unsigned long long)(unsigned char)15ull >> ((unsigned)(unsigned int)g4 & 63)) : (int)((unsigned int)(unsigned char)ga0[1] * (unsigned int)(short)100ull))) * (unsigned int)(unsigned char)(((long long)0ull <= (long long)2ull)));
    m1 = (unsigned int)(((unsigned short)2ull < (signed char)g6));
    { int w0 = 1; while (w0 > 0) {
        { int w1 = 5; while (w1 > 0) {
            g5 = (unsigned char)g7;
            w1--;
        } }
        w0--;
    } }
    if (((unsigned short)((unsigned int)(unsigned char)g8 & (unsigned int)(unsigned int)15ull) <= (unsigned int)((unsigned int)(unsigned int)g6 - (unsigned int)(short)15ull)) || ((unsigned short)2ull < (signed char)((unsigned int)(short)59929ull ^ (unsigned int)(unsigned char)g7))) {
        if ((unsigned long long)15ull == (short)((unsigned int)(unsigned short)m2 ^ (unsigned int)(signed char)m0)) {
            for (int i2 = 0; i2 < 2; i2++) {
                g1 = (unsigned long long)((unsigned long long)(unsigned short)127ull >> ((unsigned)(unsigned int)2161648238ull & 63));
            }
            g0 = (long long)g0;
            { int w3 = 4; while (w3 > 0) {
                g4 = (unsigned int)((unsigned int)(unsigned char)(((unsigned char)g4 == (unsigned short)((unsigned int)(unsigned long long)128ull % ((unsigned int)(unsigned char)3ull | 1u)))) + (unsigned int)(unsigned short)ga1[3]);
                w3--;
            } }
        }
        for (int i4 = 0; i4 < 4; i4++) {
            g7 = (unsigned char)((unsigned int)(unsigned char)((unsigned int)(unsigned char)(((short)54953ull <= (long long)0ull)) - (unsigned int)(unsigned char)((unsigned int)(unsigned int)32767ull >> ((unsigned)(unsigned int)ga0[1] & 31))) >> ((unsigned)(unsigned int)(~(unsigned int)(signed char)g6) & 31));
            switch ((int)((int)((unsigned int)(unsigned char)((unsigned int)(unsigned char)77ull >> ((unsigned)(unsigned int)m0 & 31)) * (unsigned int)(short)15ull)) & 7) {
            case 1:
                g7 = (unsigned char)((unsigned int)(unsigned int)((unsigned int)(unsigned short)249ull - (unsigned int)(unsigned int)g3) << ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(short)((unsigned int)(unsigned char)ga1[4] * (unsigned int)(long long)g8)) & 31));
            case 7:
                g7 = (unsigned char)f1((short)(~(unsigned int)(unsigned long long)3ull), (unsigned char)((unsigned int)(short)m2 + (unsigned int)(signed char)g7), (unsigned int)(((unsigned long long)g4 >= (unsigned char)32767ull) ? (int)127ull : (unsigned short)g7));
                break;
            default:
                g2 = (signed char)g7;
            }
            g2 = (signed char)f0((signed char)1ull, (int)g8, (unsigned int)1ull, (short)1ull);
        }
    }
    switch ((int)((int)m2) & 7) {
    case 3:
        ga1[4] = (unsigned int)((unsigned int)(unsigned short)g2 ^ (unsigned int)(short)ga1[5]);
    case 6:
        switch ((int)((int)65535ull) & 7) {
        case 5:
            { int w5 = 2; while (w5 > 0) {
                g3 = (unsigned long long)((unsigned long long)(signed char)2ull >> ((unsigned)(unsigned int)((unsigned int)(unsigned short)256ull >> ((unsigned)(unsigned int)65535ull & 31)) & 63));
                w5--;
            } }
            break;
        case 6:
            g8 = (int)g2;
        default:
            g1 = (unsigned long long)((unsigned long long)(unsigned int)32767ull * (unsigned long long)(signed char)ga0[3]);
        }
    case 7:
        ga0[1] = (int)((unsigned int)0 - (unsigned int)(signed char)((unsigned int)(int)3ull / ((unsigned int)(long long)((unsigned long long)(long long)ga1[6] * (unsigned long long)(unsigned long long)32767ull) | 1u)));
    default:
        for (int i6 = 0; i6 < 4; i6++) {
            ga1[5] = (unsigned int)((((int)(((unsigned short)m0 < (unsigned short)127ull)) >= (unsigned char)((unsigned int)(signed char)127ull >> ((unsigned)(unsigned int)12ull & 31))) || ((short)((unsigned int)(unsigned char)ga0[5] + (unsigned int)(unsigned int)g2) != (unsigned char)(((unsigned long long)65535ull <= (int)15ull)))));
            if ((unsigned int)ga0[5] >= (signed char)(~(unsigned int)(long long)127ull)) {
                m2 = (signed char)7ull;
                g8 = (int)((unsigned int)(short)((unsigned int)(unsigned short)128ull | (unsigned int)(long long)128ull) >> ((unsigned)(unsigned int)((((unsigned int)100ull > (long long)ga0[0]) && ((short)i6)) ? (long long)127ull : (long long)65535ull) & 31));
                g5 = (unsigned char)(((unsigned short)g5 == (unsigned long long)ga1[4]) ? (unsigned long long)100ull : (signed char)g5);
            }
        }
    }
    for (int i7 = 0; i7 < 5; i7++) {
        m1 = (unsigned int)(((long long)g8 > (unsigned long long)((unsigned long long)(unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(unsigned char)255ull >> ((unsigned)(unsigned int)m2 & 63)) % ((unsigned int)(signed char)f1((short)ga0[3], (unsigned char)g5, (unsigned int)1ull) | 1u)) * (unsigned long long)(unsigned int)2814563475ull)));
    }
    if ((((long long)m0 >= (long long)g8) && ((int)g1 <= (signed char)m1)) && ((((((long long)ga1[4] == (short)3ull) || (!((unsigned char)g7 > (unsigned char)m0))) && ((unsigned long long)65535ull >= (unsigned char)g5)) || ((unsigned long long)15ull == (int)ga1[5])) && (!((short)ga1[4] == (long long)127ull)))) {
        switch ((int)((int)(~(unsigned int)(long long)f1((short)g1, (unsigned char)ga0[0], (unsigned int)ga0[1]))) & 7) {
        case 3:
            m2 = (signed char)((!((signed char)256ull == (signed char)ga0[3])));
            break;
        case 5:
            if (((unsigned char)(((int)128ull != (signed char)g6)) != (int)g1) && ((unsigned int)((unsigned int)(unsigned char)105ull & (unsigned int)(unsigned short)ga0[3]) >= (unsigned int)((unsigned int)(unsigned long long)2ull & (unsigned int)(unsigned short)1ull))) {
                g3 = (unsigned long long)ga0[5];
            }
        default:
            { int w8 = 2; while (w8 > 0) {
                g3 = (unsigned long long)f0((signed char)ga1[3], (int)407216765ull, (unsigned int)((unsigned int)(long long)100ull / ((unsigned int)(unsigned int)m1 | 1u)), (short)255ull);
                w8--;
            } }
        }
        if ((long long)((unsigned long long)(unsigned long long)(((((unsigned short)1ull < (unsigned char)g7) && ((long long)15ull < (unsigned long long)ga0[3])) && ((unsigned char)m1 >= (int)g0)) ? (signed char)g6 : (short)g7) - (unsigned long long)(unsigned int)(~(unsigned int)(unsigned long long)ga0[2])) == (signed char)(~(unsigned int)(short)(((((signed char)1ull == (short)g6) && ((unsigned char)90ull)) && ((unsigned long long)ga0[5]))))) {
            g7 = (unsigned char)((unsigned int)(unsigned int)((unsigned int)(unsigned char)ga1[3] << ((unsigned)(unsigned int)3131642925ull & 31)) | (unsigned int)(unsigned int)((unsigned int)(unsigned char)g1 & (unsigned int)(signed char)g1));
            g1 = (unsigned long long)((unsigned long long)(unsigned int)m1 | (unsigned long long)(long long)255ull);
            for (int i9 = 0; i9 < 1; i9++) {
                g7 = (unsigned char)(~(unsigned int)(long long)(((signed char)(((unsigned long long)g6 > (unsigned long long)ga1[0]) ? (signed char)m0 : (int)ga1[3]) != (unsigned char)m1)));
                g1 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(short)2ull << ((unsigned)(unsigned int)g6 & 63)) >> ((unsigned)(unsigned int)g6 & 63)) * (unsigned long long)(unsigned long long)((unsigned long long)0 - (unsigned long long)(long long)((unsigned long long)(unsigned long long)g0 & (unsigned long long)(long long)161ull)));
            }
        } else {
            m0 = (unsigned int)((unsigned int)(int)(~(unsigned int)(int)g5) & (unsigned int)(unsigned char)((unsigned int)(short)((unsigned int)(long long)ga1[1] / ((unsigned int)(int)m2 | 1u)) >> ((unsigned)(unsigned int)(((int)2ull == (int)g0) ? (int)m0 : (unsigned char)ga0[3]) & 31)));
            { int w10 = 1; while (w10 > 0) {
                g5 = (unsigned char)(((short)w10 < (unsigned char)g1) ? (unsigned int)255ull : (unsigned short)0ull);
                w10--;
            } }
            g1 = (unsigned long long)((!(!(((unsigned long long)3ull != (unsigned char)ga1[0]) && ((long long)g5 > (short)ga0[5])))) ? (int)ga0[0] : (unsigned long long)1ull);
        }
    } else {
        m1 = (unsigned int)f0((signed char)((unsigned int)(short)f1((short)7ull, (unsigned char)m0, (unsigned int)g5) ^ (unsigned int)(unsigned int)((unsigned int)(short)m0 ^ (unsigned int)(unsigned char)ga0[0])), (int)((unsigned int)(unsigned int)ga1[1] << ((unsigned)(unsigned int)ga1[4] & 31)), (unsigned int)((unsigned int)(unsigned char)m0 / ((unsigned int)(short)(((int)7ull != (unsigned short)g1) ? (long long)3ull : (unsigned short)2ull) | 1u)), (short)(((unsigned short)((unsigned int)(unsigned short)15ull >> ((unsigned)(unsigned int)m2 & 31)) != (unsigned int)((unsigned int)(long long)15ull | (unsigned int)(signed char)g5))));
        if (((signed char)((unsigned int)(int)g2 - (unsigned int)(unsigned char)ga0[2]) <= (short)((!((signed char)g3 > (long long)100ull)) ? (long long)g6 : (int)128ull)) && ((((int)256ull >= (unsigned long long)g8) && ((signed char)m1)) || ((unsigned char)g6 < (signed char)g7))) {
            for (int i11 = 0; i11 < 1; i11++) {
                m0 = (unsigned int)((unsigned int)(unsigned int)g8 >> ((unsigned)(unsigned int)ga1[1] & 31));
            }
            for (int i12 = 0; i12 < 6; i12++) {
                ga0[3] = (int)((unsigned int)(unsigned short)246ull >> ((unsigned)(unsigned int)((unsigned int)(unsigned char)ga1[5] >> ((unsigned)(unsigned int)((unsigned int)(signed char)m1 % ((unsigned int)(unsigned long long)m1 | 1u)) & 31)) & 31));
                g8 = (int)((unsigned int)(short)7ull - (unsigned int)(unsigned short)m0);
            }
            { int w13 = 3; while (w13 > 0) {
                g2 = (signed char)((unsigned int)(unsigned short)255ull & (unsigned int)(unsigned int)g4);
                w13--;
            } }
        }
        for (int i14 = 0; i14 < 5; i14++) {
            { int w15 = 4; while (w15 > 0) {
                g2 = (signed char)(((((unsigned int)((((unsigned short)w15 > (long long)g6) || ((unsigned int)32767ull == (unsigned int)i14)))) && ((((unsigned char)m0 != (unsigned int)m2) || ((short)g1 > (unsigned short)ga0[4])) && ((int)100ull < (unsigned long long)g7))) || ((((unsigned long long)g7) || (((unsigned char)3ull) && ((signed char)255ull <= (unsigned int)3ull))) && ((unsigned int)(~(unsigned int)(unsigned int)ga0[4]) < (unsigned int)((unsigned int)(signed char)65535ull & (unsigned int)(short)m0)))));
                w15--;
            } }
            if ((long long)((unsigned long long)(signed char)((unsigned int)(unsigned short)127ull % ((unsigned int)(short)ga0[5] | 1u)) + (unsigned long long)(unsigned long long)((unsigned long long)(int)ga0[4] | (unsigned long long)(unsigned char)m1)) >= (signed char)f0((signed char)((unsigned int)(unsigned int)32767ull - (unsigned int)(unsigned long long)g3), (int)((unsigned int)(long long)g2 | (unsigned int)(unsigned int)127ull), (unsigned int)((((long long)ga0[5] > (short)m0) && ((signed char)ga1[5] >= (int)g2)) ? (long long)32767ull : (unsigned long long)g2), (short)((unsigned int)(long long)128ull | (unsigned int)(signed char)g6))) {
                g7 = (unsigned char)((unsigned int)0 - (unsigned int)(int)((unsigned int)(int)g8 - (unsigned int)(long long)i14));
                ga0[0] = (int)1ull;
                m1 = (unsigned int)((unsigned int)(unsigned short)((unsigned int)(int)f0((signed char)3ull, (int)g4, (unsigned int)m0, (short)g0) ^ (unsigned int)(unsigned long long)ga0[3]) * (unsigned int)(short)f1((short)(((((((unsigned short)10ull) || ((unsigned long long)3ull != (unsigned long long)g2)) || (((int)ga0[1] == (signed char)65535ull) && ((unsigned int)i14 != (unsigned int)7ull))) && (((unsigned char)m1 > (signed char)g1) || ((long long)ga0[4] < (unsigned char)g2))) || ((long long)255ull <= (signed char)i14))), (unsigned char)((unsigned int)(unsigned char)m0 & (unsigned int)(unsigned short)g8), (unsigned int)((unsigned int)(unsigned int)g6 ^ (unsigned int)(unsigned short)g6)));
            } else {
                ga0[4] = (int)(~(unsigned int)(short)((unsigned int)(short)(((short)ga1[0] <= (int)g1)) * (unsigned int)(short)((unsigned int)(int)128ull * (unsigned int)(int)32ull)));
                g6 = (unsigned int)((unsigned int)(signed char)((unsigned int)(short)ga0[0] * (unsigned int)(signed char)7ull) | (unsigned int)(long long)((unsigned long long)(signed char)ga0[0] | (unsigned long long)(unsigned char)ga1[1]));
                g1 = (unsigned long long)((unsigned long long)(unsigned short)f1((short)g6, (unsigned char)ga0[2], (unsigned int)52ull) & (unsigned long long)(unsigned int)256ull);
            }
        }
    }
    if ((unsigned short)(((unsigned char)((unsigned int)(long long)2ull * (unsigned int)(long long)m1) >= (int)g2)) <= (unsigned long long)m1) {
        g6 = (unsigned int)ga1[0];
        if ((signed char)((unsigned int)0 - (unsigned int)(int)((unsigned int)(unsigned int)g7 & (unsigned int)(long long)ga0[2])) != (long long)((unsigned long long)(unsigned char)ga1[6] - (unsigned long long)(signed char)((unsigned int)(unsigned char)ga1[0] % ((unsigned int)(long long)g3 | 1u)))) {
            g3 = (unsigned long long)((unsigned long long)(long long)((unsigned long long)(short)ga0[3] + (unsigned long long)(long long)g2) - (unsigned long long)(unsigned short)((unsigned int)(short)15ull - (unsigned int)(unsigned long long)g1));
        }
    } else {
        m2 = (signed char)((unsigned int)(short)m1 - (unsigned int)(unsigned int)((unsigned int)(long long)((unsigned long long)(unsigned short)(~(unsigned int)(unsigned long long)3ull) - (unsigned long long)(unsigned int)g7) % ((unsigned int)(signed char)((unsigned int)(unsigned short)((unsigned int)(unsigned int)ga0[2] ^ (unsigned int)(signed char)2ull) % ((unsigned int)(unsigned char)((unsigned int)(short)45780ull | (unsigned int)(unsigned short)0ull) | 1u)) | 1u)));
    }
    if ((short)((unsigned int)(unsigned char)106ull | (unsigned int)(unsigned int)((unsigned int)(long long)ga1[1] << ((unsigned)(unsigned int)ga0[4] & 31))) >= (int)(~(unsigned int)(unsigned char)f0((signed char)m2, (int)ga1[5], (unsigned int)2ull, (short)145ull))) {
        m2 = (signed char)(((!(((long long)g8 <= (unsigned long long)ga1[6]) || ((unsigned int)ga0[2] > (long long)65535ull))) && ((!(((unsigned char)m0 > (int)100ull) && ((unsigned int)ga1[4] > (unsigned short)7ull))) || ((unsigned short)ga0[0] == (short)128ull))));
        g6 = (unsigned int)g4;
        switch ((int)((int)((unsigned int)(unsigned short)(((!((signed char)m1 >= (signed char)m2)) && ((unsigned char)g4)) ? (int)g7 : (short)15ull) & (unsigned int)(unsigned short)(((unsigned int)194ull <= (unsigned int)m0) ? (signed char)177ull : (signed char)m1))) & 7) {
        case 0:
            g3 = (unsigned long long)((unsigned long long)(short)g1 >> ((unsigned)(unsigned int)(((int)15ull <= (unsigned short)g7)) & 63));
        case 2:
            g1 = (unsigned long long)((unsigned long long)(unsigned int)164ull / ((unsigned long long)(unsigned int)3ull | 1u));
            break;
        case 3:
            g3 = (unsigned long long)((unsigned long long)(unsigned short)((unsigned int)(signed char)(((unsigned long long)(~(unsigned long long)(int)ga0[0]) != (signed char)ga1[6]) ? (signed char)(((signed char)256ull <= (unsigned char)g0) ? (unsigned long long)ga0[0] : (int)g4) : (unsigned long long)((unsigned long long)(unsigned int)100ull / ((unsigned long long)(short)100ull | 1u))) >> ((unsigned)(unsigned int)((unsigned int)(int)(((short)1ull < (signed char)g8) ? (short)213ull : (unsigned short)ga0[2]) % ((unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(long long)g2) | 1u)) & 31)) << ((unsigned)(unsigned int)((unsigned int)(signed char)128ull & (unsigned int)(unsigned int)((unsigned int)(unsigned long long)(~(unsigned long long)(short)ga0[1]) << ((unsigned)(unsigned int)g8 & 31))) & 63));
            break;
        case 6:
            m0 = (unsigned int)(((unsigned short)((unsigned int)(short)ga1[0] >> ((unsigned)(unsigned int)g5 & 31)) < (unsigned short)32767ull) ? (unsigned short)ga1[0] : (short)((unsigned int)(unsigned char)65535ull + (unsigned int)(long long)ga1[6]));
            break;
        default:
            m0 = (unsigned int)((unsigned int)(long long)ga0[1] - (unsigned int)(long long)15ull);
        }
    }
    if ((long long)((unsigned long long)(unsigned short)((unsigned int)(signed char)g4 & (unsigned int)(signed char)g0) | (unsigned long long)(short)(~(unsigned int)(unsigned long long)g6)) <= (short)((unsigned int)0 - (unsigned int)(short)118ull)) {
        switch ((int)((int)(~(unsigned int)(unsigned short)((unsigned int)(int)7ull >> ((unsigned)(unsigned int)241ull & 31)))) & 7) {
        case 4:
            switch ((int)((int)ga1[4]) & 7) {
            case 1:
                g3 = (unsigned long long)((unsigned long long)(int)(((((unsigned long long)32767ull) || (((int)2ull == (unsigned short)ga1[5]) || ((unsigned int)ga0[2] <= (short)g3))) && (!((short)ga0[3])))) >> ((unsigned)(unsigned int)((((signed char)m0 <= (int)f0((signed char)7ull, (int)2ull, (unsigned int)g1, (short)ga0[1])) && ((unsigned char)f1((short)256ull, (unsigned char)15ull, (unsigned int)g6) < (unsigned char)127ull))) & 63));
                break;
            case 2:
                g2 = (signed char)g0;
                break;
            case 6:
                m1 = (unsigned int)(~(unsigned int)(unsigned char)((unsigned int)(unsigned char)111ull / ((unsigned int)(unsigned int)g1 | 1u)));
                break;
            default:
                ga1[2] = (unsigned int)((unsigned int)(unsigned int)((unsigned int)(unsigned long long)ga1[4] / ((unsigned int)(unsigned long long)((unsigned long long)(int)ga1[3] ^ (unsigned long long)(int)g3) | 1u)) + (unsigned int)(signed char)((unsigned int)(long long)f0((signed char)32767ull, (int)m0, (unsigned int)1ull, (short)46049ull) - (unsigned int)(unsigned int)(((short)15ull > (long long)m0))));
            }
        case 6:
            g5 = (unsigned char)((unsigned int)0 - (unsigned int)(long long)g2);
            break;
        case 7:
            ga1[4] = (unsigned int)((unsigned int)(int)((unsigned int)(short)(~(unsigned int)(long long)g3) / ((unsigned int)(signed char)(((short)m0 > (signed char)g7) ? (int)65535ull : (unsigned int)ga1[0]) | 1u)) >> ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(signed char)((unsigned int)(int)g8 / ((unsigned int)(unsigned int)100864723ull | 1u))) & 31));
            break;
        default:
            g7 = (unsigned char)((unsigned int)(unsigned short)g8 - (unsigned int)(unsigned int)ga0[3]);
        }
        g5 = (unsigned char)((((signed char)256ull < (int)ga0[0]) && ((short)g7)));
        if ((unsigned short)((unsigned int)(signed char)g5 * (unsigned int)(unsigned int)((unsigned int)(signed char)g1 / ((unsigned int)(unsigned char)15ull | 1u))) > (signed char)((unsigned int)(unsigned char)((unsigned int)(unsigned int)1ull + (unsigned int)(unsigned char)g4) / ((unsigned int)(int)g2 | 1u))) {
            if (((unsigned long long)((unsigned long long)(long long)g8 ^ (unsigned long long)(short)256ull)) && ((unsigned char)((((unsigned char)15ull) && ((((unsigned char)2ull) || (((unsigned char)g0 > (short)1ull) || (((signed char)ga0[3] > (unsigned short)m1) && ((long long)7ull < (int)ga0[3])))) || ((((int)ga1[6]) && ((unsigned short)ga1[2] > (int)32767ull)) || ((signed char)m0 != (unsigned char)ga1[4])))) ? (unsigned int)32767ull : (unsigned long long)g0) >= (unsigned long long)((((((signed char)127ull > (unsigned long long)7ull) || ((((int)g5 < (long long)g5) && (((signed char)216ull != (unsigned long long)ga1[2]) || (!((unsigned long long)255ull <= (signed char)g3)))) || (!((unsigned short)m2 <= (unsigned char)m1)))) || (((short)ga1[5] != (unsigned long long)ga1[1]) && ((((unsigned short)g0 != (short)3ull) && ((int)ga0[3] != (unsigned short)64675ull)) || ((short)ga0[2] != (unsigned long long)65535ull)))) || ((unsigned short)m0 >= (signed char)100ull)) ? (unsigned char)ga1[3] : (unsigned long long)g5))) {
                g5 = (unsigned char)((unsigned int)(int)((unsigned int)(int)ga1[1] & (unsigned int)(unsigned char)g6) & (unsigned int)(unsigned long long)((unsigned long long)(signed char)15ull - (unsigned long long)(unsigned int)m0));
                ga0[3] = (int)((((signed char)(((unsigned short)g2 == (long long)0ull)) != (unsigned short)((unsigned int)(unsigned char)ga1[2] * (unsigned int)(unsigned long long)255ull)) && (((int)ga1[6] < (int)3ull) || ((short)128ull <= (unsigned int)m0))) ? (signed char)((unsigned int)(signed char)((((unsigned char)32767ull < (short)1ull) && (((long long)255ull >= (int)ga1[3]) && (((unsigned char)g5) && (((unsigned long long)ga0[4] > (unsigned long long)g7) && ((unsigned char)0ull >= (signed char)g8)))))) ^ (unsigned int)(unsigned char)((unsigned int)(signed char)100ull | (unsigned int)(int)15ull)) : (signed char)((unsigned int)(long long)((unsigned long long)(signed char)g8 << ((unsigned)(unsigned int)ga1[0] & 63)) ^ (unsigned int)(unsigned short)f0((signed char)ga0[5], (int)g0, (unsigned int)g5, (short)g6)));
            } else {
                g2 = (signed char)100ull;
                g2 = (signed char)((unsigned int)(int)((unsigned int)(short)((unsigned int)(int)m2 | (unsigned int)(signed char)g8) >> ((unsigned)(unsigned int)((unsigned int)(short)g5 ^ (unsigned int)(unsigned long long)g1) & 31)) - (unsigned int)(unsigned int)g3);
            }
            m2 = (signed char)(~(unsigned int)(short)f0((signed char)(~(unsigned int)(unsigned short)3ull), (int)(((int)g2 == (int)g6)), (unsigned int)((unsigned int)(int)255ull >> ((unsigned)(unsigned int)3ull & 31)), (short)(((signed char)0ull) ? (unsigned long long)229ull : (unsigned long long)m0)));
        }
    } else {
        if ((signed char)g7) {
            for (int i16 = 0; i16 < 1; i16++) {
                g2 = (signed char)((unsigned int)(unsigned char)g0 << ((unsigned)(unsigned int)((unsigned int)(int)4133949607ull - (unsigned int)(unsigned char)m2) & 31));
                g8 = (int)(~(unsigned int)(unsigned int)((unsigned int)(unsigned long long)g2 / ((unsigned int)(signed char)65535ull | 1u)));
            }
            { int w17 = 2; while (w17 > 0) {
                g8 = (int)((unsigned int)(int)((unsigned int)(long long)g7 + (unsigned int)(unsigned long long)(~(unsigned long long)(long long)((unsigned long long)(signed char)127ull << ((unsigned)(unsigned int)g3 & 63)))) ^ (unsigned int)(short)((unsigned int)(unsigned char)((unsigned int)(int)((unsigned int)(unsigned long long)ga0[3] + (unsigned int)(unsigned int)7ull) << ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(unsigned char)g2) & 31)) + (unsigned int)(int)(~(unsigned int)(long long)((unsigned long long)(unsigned long long)1ull >> ((unsigned)(unsigned int)7ull & 63)))));
                w17--;
            } }
            g0 = (long long)(((unsigned long long)(~(unsigned long long)(int)g1) <= (unsigned int)((unsigned int)(unsigned short)0ull ^ (unsigned int)(unsigned short)ga0[0])));
        }
        for (int i18 = 0; i18 < 3; i18++) {
            g3 = (unsigned long long)(((short)((unsigned int)(unsigned long long)((unsigned long long)(unsigned char)(((unsigned int)32767ull == (long long)7ull)) - (unsigned long long)(int)((((long long)ga0[1] <= (signed char)7ull) && ((int)ga0[4] == (unsigned long long)32767ull)) ? (long long)100ull : (unsigned short)g3)) >> ((unsigned)(unsigned int)f0((signed char)((unsigned int)(unsigned int)g4 * (unsigned int)(int)g6), (int)f0((signed char)ga1[3], (int)g5, (unsigned int)g5, (short)m2), (unsigned int)((unsigned int)(long long)7ull % ((unsigned int)(unsigned short)g3 | 1u)), (short)((unsigned int)(unsigned char)128ull - (unsigned int)(unsigned short)100ull)) & 31)) <= (long long)((unsigned long long)(short)((unsigned int)(signed char)(~(unsigned int)(long long)127ull) / ((unsigned int)(signed char)((unsigned int)(long long)ga1[0] << ((unsigned)(unsigned int)g0 & 31)) | 1u)) | (unsigned long long)(unsigned int)128ull)));
            switch ((int)((int)147ull) & 7) {
            case 0:
                ga1[6] = (unsigned int)((unsigned int)(signed char)((unsigned int)0 - (unsigned int)(signed char)((unsigned int)(unsigned int)g7 - (unsigned int)(int)3ull)) ^ (unsigned int)(short)((unsigned int)(short)((unsigned int)(unsigned char)m2 & (unsigned int)(unsigned long long)ga1[6]) * (unsigned int)(unsigned char)g7));
            case 1:
                m2 = (signed char)((unsigned int)(unsigned int)127ull | (unsigned int)(unsigned int)g2);
                break;
            case 3:
                g7 = (unsigned char)((((short)ga1[1] >= (unsigned long long)256ull) && ((!((((unsigned int)ga0[4] >= (int)g3) && ((int)m2 == (unsigned int)i18)) && ((unsigned char)0ull != (unsigned short)ga1[3]))) || ((!((unsigned short)0ull >= (signed char)0ull)) || (((unsigned int)32767ull == (unsigned char)m1) || ((unsigned int)3ull >= (unsigned long long)g2))))));
                break;
            case 7:
                g5 = (unsigned char)ga1[1];
                break;
            default:
                g6 = (unsigned int)((unsigned int)(short)(~(unsigned int)(unsigned short)((unsigned int)(unsigned char)g8 ^ (unsigned int)(short)((unsigned int)(short)g2 ^ (unsigned int)(unsigned long long)1739525039243775473ull))) * (unsigned int)(unsigned short)((!(((unsigned char)256ull >= (short)7ull) || ((signed char)m2 < (unsigned int)7ull))) ? (short)((((int)ga0[2] < (unsigned char)m1) && ((short)i18 <= (signed char)m1))) : (unsigned long long)((unsigned long long)(unsigned char)f1((short)128ull, (unsigned char)g4, (unsigned int)100ull) * (unsigned long long)(unsigned char)g2)));
            }
            g8 = (int)((unsigned int)(short)((unsigned int)(int)g5 ^ (unsigned int)(long long)100ull) ^ (unsigned int)(signed char)ga1[4]);
        }
        ga0[4] = (int)((unsigned int)(signed char)f0((signed char)f0((signed char)15ull, (int)5478797ull, (unsigned int)1ull, (short)0ull), (int)((unsigned int)(short)65535ull * (unsigned int)(signed char)96ull), (unsigned int)((unsigned int)(unsigned char)g7 % ((unsigned int)(unsigned int)15ull | 1u)), (short)f1((short)ga0[2], (unsigned char)77ull, (unsigned int)136ull)) + (unsigned int)(signed char)((unsigned int)(unsigned short)((unsigned int)(unsigned char)228ull ^ (unsigned int)(unsigned long long)g0) & (unsigned int)(unsigned short)f1((short)m2, (unsigned char)ga0[4], (unsigned int)609156487ull)));
    }
    PR("g0", g0);
    PR("g1", g1);
    PR("g2", g2);
    PR("g3", g3);
    PR("g4", g4);
    PR("g5", g5);
    PR("g6", g6);
    PR("g7", g7);
    PR("g8", g8);
    PR("m0", m0);
    PR("m1", m1);
    PR("m2", m2);
    PR("ga0_0", ga0[0]);
    PR("ga0_1", ga0[1]);
    PR("ga0_2", ga0[2]);
    PR("ga0_3", ga0[3]);
    PR("ga0_4", ga0[4]);
    PR("ga0_5", ga0[5]);
    PR("ga1_0", ga1[0]);
    PR("ga1_1", ga1[1]);
    PR("ga1_2", ga1[2]);
    PR("ga1_3", ga1[3]);
    PR("ga1_4", ga1[4]);
    PR("ga1_5", ga1[5]);
    PR("ga1_6", ga1[6]);
    return 0;
}

