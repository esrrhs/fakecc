package main;
import runtime;
signed char g0 = (signed char)15ull;
signed char g1 = (signed char)7ull;
short g2 = (short)58ull;
unsigned short g3 = (unsigned short)56219ull;
signed char g4 = (signed char)15ull;
signed char g5 = (signed char)78ull;
short g6 = (short)100ull;
unsigned int ga0[4] = {(unsigned int)127ull, (unsigned int)127ull, (unsigned int)255ull, (unsigned int)127ull};
unsigned char ga1[5] = {(unsigned char)80ull, (unsigned char)127ull, (unsigned char)103ull, (unsigned char)1ull, (unsigned char)125ull};
static signed char f0(short p0, short p1) {
    unsigned long long l0 = (unsigned long long)((unsigned long long)(long long)p1 + (unsigned long long)(int)((unsigned int)(unsigned int)2ull << ((unsigned)(unsigned int)ga1[3] & 31)));
    short l1 = (short)((unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(int)g1) & (unsigned int)(unsigned long long)((unsigned long long)(unsigned char)115ull * (unsigned long long)(signed char)ga0[3]));
    g0 = (signed char)((unsigned int)(unsigned short)l0 & (unsigned int)(unsigned long long)g4);
    for (int i0 = 0; i0 < 5; i0++) {
        g6 = (short)(((((signed char)g6 < (unsigned int)g0) && ((unsigned short)((unsigned int)(unsigned short)g5 >> ((unsigned)(unsigned int)g0 & 31)) == (unsigned int)((unsigned int)(short)p1 + (unsigned int)(unsigned char)ga1[2]))) || (!(!((unsigned int)128ull == (unsigned long long)g1)))) ? (long long)(((short)((unsigned int)(unsigned short)((((unsigned short)ga0[3] == (short)ga1[3]) || (!((long long)7ull))) ? (unsigned int)ga0[3] : (unsigned short)3ull) << ((unsigned)(unsigned int)((unsigned int)(unsigned long long)g3 ^ (unsigned int)(unsigned int)0ull) & 31)) > (long long)(~(unsigned long long)(short)((((unsigned long long)ga1[4] <= (long long)32767ull) && ((signed char)g6 != (short)0ull)) ? (unsigned long long)g5 : (unsigned long long)ga1[3]))) ? (unsigned long long)((unsigned long long)(short)((unsigned int)(unsigned int)ga1[4] & (unsigned int)(unsigned char)65535ull) / ((unsigned long long)(long long)((((unsigned long long)69ull >= (unsigned char)3ull) && ((int)ga0[3] != (unsigned short)ga1[4]))) | 1u)) : (unsigned char)(~(unsigned int)(long long)((unsigned long long)(long long)p0 * (unsigned long long)(long long)p0))) : (unsigned short)((unsigned int)(long long)((unsigned long long)(unsigned long long)(((unsigned char)7ull > (unsigned short)127ull)) - (unsigned long long)(short)((unsigned int)0 - (unsigned int)(unsigned long long)ga0[1])) % ((unsigned int)(unsigned int)((unsigned int)0 - (unsigned int)(signed char)((unsigned int)(unsigned int)ga1[4] + (unsigned int)(int)l0)) | 1u)));
    }
    { int w1 = 3; while (w1 > 0) {
        l1 = (short)p0;
        w1--;
    } }
    switch ((int)((int)(((unsigned char)((unsigned int)(unsigned char)256ull - (unsigned int)(long long)p1) >= (unsigned int)((unsigned int)(short)7ull << ((unsigned)(unsigned int)ga0[3] & 31))) ? (signed char)p0 : (unsigned short)((((unsigned short)g3 != (long long)ga1[0]) && ((short)255ull > (unsigned int)l1))))) & 7) {
    case 1:
        if ((int)(~(unsigned int)(long long)((unsigned long long)(long long)ga0[0] | (unsigned long long)(unsigned long long)g2)) <= (unsigned short)((unsigned int)(short)g4 * (unsigned int)(long long)(((signed char)256ull < (unsigned int)p0) ? (unsigned int)l1 : (unsigned int)g0))) {
            ga1[2] = (unsigned char)((unsigned int)(long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned short)g1 ^ (unsigned int)(unsigned char)15ull) % ((unsigned long long)(unsigned char)((unsigned int)(long long)g6 ^ (unsigned int)(long long)0ull) | 1u)) - (unsigned int)(signed char)ga1[2]);
            p0 = (short)(((signed char)128ull > (unsigned int)255ull));
            g2 = (short)((unsigned int)(long long)((unsigned long long)(int)((unsigned int)(long long)0ull / ((unsigned int)(unsigned short)ga0[2] | 1u)) & (unsigned long long)(signed char)((unsigned int)0 - (unsigned int)(unsigned short)ga1[0])) >> ((unsigned)(unsigned int)g2 & 31));
        } else {
            g5 = (signed char)((unsigned int)(short)((unsigned int)(signed char)65ull - (unsigned int)(unsigned short)g6) * (unsigned int)(int)((unsigned int)(signed char)l0 << ((unsigned)(unsigned int)p1 & 31)));
            ga0[2] = (unsigned int)ga1[4];
            g5 = (signed char)g0;
        }
        break;
    case 4:
        g4 = (signed char)(~(unsigned int)(unsigned short)((unsigned int)(signed char)g2 + (unsigned int)(long long)l0));
        break;
    default:
        if ((unsigned long long)(((unsigned short)((unsigned int)(unsigned char)32767ull | (unsigned int)(unsigned short)g5) >= (unsigned int)((unsigned int)0 - (unsigned int)(unsigned long long)p0))) != (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(unsigned char)7ull * (unsigned long long)(short)l0) * (unsigned long long)(unsigned long long)((unsigned long long)(long long)65535ull + (unsigned long long)(signed char)ga1[1]))) {
            l1 = (short)p0;
            g1 = (signed char)(~(unsigned int)(unsigned int)((unsigned int)(unsigned long long)256ull / ((unsigned int)(short)g0 | 1u)));
            ga0[2] = (unsigned int)((unsigned int)(short)(((unsigned char)((unsigned int)0 - (unsigned int)(long long)100ull)) ? (unsigned int)((unsigned int)(unsigned long long)ga0[0] + (unsigned int)(long long)p0) : (short)((((int)g4 < (unsigned char)32767ull) || ((unsigned short)9ull)))) | (unsigned int)(int)(((unsigned long long)p0 != (int)g5)));
        }
    }
    switch ((int)((int)((unsigned int)(unsigned short)((unsigned int)(long long)32767ull * (unsigned int)(unsigned char)p0) / ((unsigned int)(int)((unsigned int)(long long)g6 / ((unsigned int)(short)3ull | 1u)) | 1u))) & 7) {
    case 2:
        g4 = (signed char)((unsigned int)(int)p0 / ((unsigned int)(signed char)ga1[3] | 1u));
    case 4:
        g2 = (short)g1;
    default:
        if ((unsigned long long)((unsigned long long)0 - (unsigned long long)(unsigned short)0ull) < (unsigned short)((unsigned int)(long long)(~(unsigned long long)(int)l0) - (unsigned int)(unsigned long long)((unsigned long long)(signed char)ga0[2] & (unsigned long long)(int)ga0[0]))) {
            ga1[4] = (unsigned char)((unsigned int)(long long)((unsigned long long)(unsigned long long)3ull & (unsigned long long)(unsigned int)128ull) ^ (unsigned int)(unsigned char)l1);
        }
    }
    g6 = (short)((unsigned int)(long long)65535ull << ((unsigned)(unsigned int)((unsigned int)(signed char)((unsigned int)(int)(~(unsigned int)(long long)g3) * (unsigned int)(int)g3) << ((unsigned)(unsigned int)((unsigned int)(unsigned long long)((unsigned long long)(unsigned short)22718ull ^ (unsigned long long)(unsigned char)233ull) / ((unsigned int)(unsigned short)((unsigned int)(signed char)p1 % ((unsigned int)(signed char)g3 | 1u)) | 1u)) & 31)) & 31));
    return (signed char)g5;
}

static signed char f1(long long p0) {
    int l0 = (int)((unsigned int)(unsigned char)(((unsigned int)g6)) * (unsigned int)(unsigned int)((unsigned int)(unsigned int)ga0[0] + (unsigned int)(unsigned int)32767ull));
    unsigned long long l1 = (unsigned long long)g4;
    short l2 = (short)((unsigned int)(unsigned char)g6 ^ (unsigned int)(int)g6);
    for (int i0 = 0; i0 < 5; i0++) {
        g4 = (signed char)(((unsigned long long)(((((long long)ga0[1] != (unsigned long long)ga0[1]) && (((unsigned char)ga0[2] > (unsigned short)0ull) || ((((short)1ull >= (unsigned long long)g0) || ((signed char)g4 > (unsigned short)g0)) || ((unsigned int)g2 >= (int)g4)))) && (((short)7ull < (unsigned long long)1ull) || ((signed char)7ull < (int)100ull))) ? (long long)ga0[1] : (int)256ull)) ? (unsigned long long)(((int)ga1[4] >= (long long)p0)) : (unsigned short)g0);
    }
    l0 = (int)(~(unsigned int)(unsigned short)255ull);
    for (int i1 = 0; i1 < 3; i1++) {
        p0 = (long long)l2;
        if ((unsigned short)((unsigned int)(unsigned short)((!((((unsigned short)g4 != (int)g3) || (((((unsigned int)i1 != (unsigned short)100ull) && (((int)3ull > (unsigned short)g5) && ((int)1ull < (unsigned int)g3))) && ((int)ga1[2] != (signed char)ga1[0])) || (((int)679431182ull < (unsigned long long)1ull) || ((short)100ull != (unsigned long long)ga1[2])))) || ((unsigned long long)1ull >= (int)g5))) ? (unsigned short)ga1[4] : (unsigned int)7ull) ^ (unsigned int)(int)((unsigned int)(int)0ull | (unsigned int)(unsigned int)1ull))) {
            ga1[2] = (unsigned char)((unsigned int)(unsigned long long)(~(unsigned long long)(long long)(((int)g6) ? (int)g1 : (unsigned long long)100ull)) - (unsigned int)(short)((unsigned int)(unsigned int)((unsigned int)(long long)g0 - (unsigned int)(int)g2) / ((unsigned int)(unsigned long long)((unsigned long long)(unsigned int)127ull * (unsigned long long)(signed char)15ull) | 1u)));
            g6 = (short)g0;
            g5 = (signed char)g5;
        } else {
            g5 = (signed char)g0;
        }
        g2 = (short)((unsigned int)(int)100ull - (unsigned int)(unsigned int)ga1[0]);
    }
    for (int i2 = 0; i2 < 3; i2++) {
        if (!(!((unsigned char)221ull < (unsigned char)ga0[1]))) {
            p0 = (long long)((unsigned long long)(long long)f0((short)256ull, (short)ga1[1]) - (unsigned long long)(unsigned long long)(((unsigned long long)15ull >= (long long)g1) ? (short)l1 : (unsigned short)ga1[3]));
            ga0[2] = (unsigned int)(((signed char)g6 == (unsigned short)2ull) ? (short)((unsigned int)(long long)((((long long)128ull > (unsigned char)ga0[3]) && (((short)127ull == (unsigned long long)i2) && ((short)g4 >= (short)255ull))) ? (unsigned char)ga0[3] : (int)32767ull) - (unsigned int)(short)(~(unsigned int)(unsigned char)127ull)) : (unsigned short)((unsigned int)(short)g1 ^ (unsigned int)(int)f0((short)0ull, (short)g6)));
            g1 = (signed char)((unsigned int)(unsigned int)g3 / ((unsigned int)(short)ga0[3] | 1u));
        }
        if ((long long)((unsigned long long)(short)(~(unsigned int)(unsigned long long)17314520983057097567ull) / ((unsigned long long)(unsigned int)((unsigned int)(unsigned short)46145ull % ((unsigned int)(signed char)ga1[4] | 1u)) | 1u)) >= (unsigned long long)((!((unsigned int)0ull != (int)g0)))) {
            l2 = (short)f0((short)((unsigned int)0 - (unsigned int)(unsigned int)((unsigned int)(unsigned long long)((unsigned long long)(unsigned char)g2 + (unsigned long long)(unsigned int)ga0[1]) << ((unsigned)(unsigned int)(~(unsigned int)(int)g0) & 31))), (short)(~(unsigned int)(unsigned int)(~(unsigned int)(unsigned int)((unsigned int)(int)ga1[4] + (unsigned int)(signed char)g1))));
            g6 = (short)(((!((int)2ull <= (unsigned char)ga1[3])) && (!((signed char)128ull > (unsigned long long)g4))));
        } else {
            g6 = (short)128ull;
            l1 = (unsigned long long)l0;
            g2 = (short)((unsigned int)(unsigned int)f0((short)256ull, (short)0ull) ^ (unsigned int)(signed char)((unsigned int)(long long)ga1[0] >> ((unsigned)(unsigned int)3ull & 31)));
        }
    }
    for (int i3 = 0; i3 < 4; i3++) {
        l0 = (int)((unsigned int)(unsigned char)((unsigned int)(signed char)i3 / ((unsigned int)(unsigned long long)g3 | 1u)) & (unsigned int)(unsigned short)((unsigned int)(unsigned char)g2 & (unsigned int)(int)ga0[1]));
        g2 = (short)ga0[3];
        { int w4 = 2; while (w4 > 0) {
            g2 = (short)((unsigned int)(unsigned int)f0((short)50798ull, (short)l0) / ((unsigned int)(unsigned long long)((!((int)ga1[2])) ? (unsigned short)128ull : (signed char)ga1[2]) | 1u));
            w4--;
        } }
    }
    return (signed char)((unsigned int)(unsigned int)(((unsigned long long)127ull != (int)ga1[0]) ? (unsigned long long)0ull : (unsigned char)0ull) & (unsigned int)(unsigned int)f0((short)65535ull, (short)g0));
}

int main(void) {
    int m0 = (int)100ull;
    long long m1 = (long long)3ull;
    unsigned int m2 = (unsigned int)2ull;
    if ((short)((unsigned int)(short)((unsigned int)(long long)ga0[3] & (unsigned int)(unsigned int)122ull) - (unsigned int)(unsigned short)((!((unsigned short)127ull <= (long long)g0)))) == (int)g5) {
        { int w0 = 4; while (w0 > 0) {
            switch ((int)((int)(~(unsigned int)(short)m1)) & 7) {
            case 1:
                g6 = (short)((unsigned int)(unsigned long long)ga0[0] / ((unsigned int)(unsigned short)ga1[4] | 1u));
            case 3:
                g2 = (short)((unsigned int)(int)((unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(unsigned short)((unsigned int)(unsigned char)ga1[2] << ((unsigned)(unsigned int)m2 & 31))) << ((unsigned)(unsigned int)g1 & 31)) | (unsigned int)(signed char)((unsigned int)(int)((unsigned int)(int)f1((long long)100ull) << ((unsigned)(unsigned int)ga1[3] & 31)) % ((unsigned int)(unsigned short)((unsigned int)(short)g4 << ((unsigned)(unsigned int)(((((unsigned int)0ull <= (long long)15ull) || ((signed char)173ull)) && ((int)127ull >= (unsigned short)ga0[0]))) & 31)) | 1u)));
                break;
            case 4:
                g4 = (signed char)(((long long)g1 < (unsigned short)w0));
                break;
            default:
                m2 = (unsigned int)((unsigned int)(short)f1((long long)m2) << ((unsigned)(unsigned int)((unsigned int)(unsigned int)m0 & (unsigned int)(short)g3) & 31));
            }
            w0--;
        } }
        { int w1 = 5; while (w1 > 0) {
            g5 = (signed char)((unsigned int)(signed char)((unsigned int)(unsigned int)(~(unsigned int)(unsigned short)127ull) & (unsigned int)(unsigned int)((unsigned int)(unsigned int)m1 & (unsigned int)(unsigned short)g0)) | (unsigned int)(unsigned short)255ull);
            w1--;
        } }
    }
    if ((unsigned short)((unsigned int)(short)((unsigned int)(unsigned long long)g3 ^ (unsigned int)(unsigned short)g5) << ((unsigned)(unsigned int)(~(unsigned int)(unsigned short)ga0[1]) & 31)) <= (int)ga1[4]) {
        g1 = (signed char)(((int)g6 >= (short)3ull) ? (unsigned char)ga0[1] : (long long)g3);
        m1 = (long long)((unsigned long long)(unsigned short)(((short)((unsigned int)(int)((unsigned int)(unsigned short)g4 * (unsigned int)(int)ga1[3]) ^ (unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(long long)32767ull)) >= (short)((unsigned int)(long long)((unsigned long long)(unsigned char)m1 / ((unsigned long long)(unsigned int)m1 | 1u)) * (unsigned int)(unsigned long long)171ull)) ? (long long)((unsigned long long)(long long)((unsigned long long)(unsigned char)g0 - (unsigned long long)(int)ga1[1]) / ((unsigned long long)(long long)((unsigned long long)(short)0ull * (unsigned long long)(short)g0) | 1u)) : (signed char)g6) | (unsigned long long)(long long)((unsigned long long)(long long)g6 - (unsigned long long)(long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned short)256ull + (unsigned int)(unsigned int)128ull) | (unsigned long long)(unsigned long long)((((unsigned int)32767ull >= (unsigned int)g2) || ((((signed char)3ull >= (unsigned short)100ull) || (((signed char)127ull <= (short)ga0[1]) || ((((unsigned int)1ull == (unsigned long long)65535ull) && ((unsigned long long)g4 < (int)ga1[1])) && ((signed char)256ull <= (long long)g3)))) && (((long long)g3 > (unsigned int)3ull) && ((unsigned int)ga0[0] >= (signed char)256ull))))))));
    } else {
        for (int i2 = 0; i2 < 6; i2++) {
            for (int i3 = 0; i3 < 5; i3++) {
                ga0[1] = (unsigned int)(((unsigned char)((unsigned int)(signed char)f0((short)7ull, (short)ga0[1]) << ((unsigned)(unsigned int)(~(unsigned int)(long long)23ull) & 31)) == (unsigned short)((unsigned int)(unsigned short)f1((long long)127ull) - (unsigned int)(unsigned short)f0((short)128ull, (short)32767ull))) ? (signed char)(~(unsigned int)(short)((unsigned int)(int)g2 & (unsigned int)(unsigned char)m0)) : (long long)((((short)ga1[4]) || ((unsigned char)g6 > (unsigned short)i3))));
                ga0[0] = (unsigned int)(((unsigned long long)(~(unsigned long long)(unsigned int)((unsigned int)(short)g0 - (unsigned int)(long long)ga1[4]))) ? (long long)f1((long long)((unsigned long long)(short)32767ull - (unsigned long long)(unsigned long long)m2)) : (long long)((unsigned long long)(unsigned short)((unsigned int)(long long)256ull & (unsigned int)(long long)65535ull) / ((unsigned long long)(unsigned long long)(((long long)15ull == (signed char)g4)) | 1u)));
            }
        }
        ga0[3] = (unsigned int)((unsigned int)(long long)(~(unsigned long long)(unsigned char)((((unsigned char)3ull) || ((long long)g0 != (short)m2)) ? (unsigned char)m1 : (int)m1)) & (unsigned int)(short)g4);
    }
    if ((unsigned int)((unsigned int)(short)ga0[3] % ((unsigned int)(long long)(((short)g1 > (unsigned int)m2) ? (unsigned short)g6 : (unsigned short)g1) | 1u)) >= (unsigned short)((unsigned int)(int)(((((int)ga0[2] < (unsigned int)ga1[3]) && ((unsigned long long)188ull < (long long)ga0[2])) && ((unsigned char)226ull <= (short)3ull))) + (unsigned int)(long long)((unsigned long long)(unsigned char)g3 * (unsigned long long)(long long)3ull))) {
        { int w4 = 4; while (w4 > 0) {
            { int w5 = 5; while (w5 > 0) {
                g4 = (signed char)((unsigned int)0 - (unsigned int)(unsigned long long)7ull);
                w5--;
            } }
            w4--;
        } }
        g3 = (unsigned short)((((unsigned int)g2 >= (unsigned char)ga0[1]) || ((((unsigned long long)g5 >= (short)m1) && ((unsigned int)15ull < (unsigned int)2ull)) && ((unsigned char)100ull <= (unsigned char)g6))));
        g5 = (signed char)ga1[0];
    } else {
        g2 = (short)m1;
        for (int i6 = 0; i6 < 4; i6++) {
            ga1[1] = (unsigned char)((unsigned int)(long long)(((unsigned char)((unsigned int)0 - (unsigned int)(unsigned int)ga0[1]) >= (unsigned short)f1((long long)87ull))) ^ (unsigned int)(signed char)((unsigned int)(unsigned int)((unsigned int)(unsigned char)i6 & (unsigned int)(unsigned int)ga1[2]) ^ (unsigned int)(signed char)(((unsigned short)ga1[1] >= (signed char)g4))));
            m2 = (unsigned int)100ull;
        }
        for (int i7 = 0; i7 < 3; i7++) {
            { int w8 = 2; while (w8 > 0) {
                ga0[1] = (unsigned int)(((unsigned long long)ga0[2]) ? (int)((unsigned int)(unsigned int)(~(unsigned int)(long long)ga1[4]) + (unsigned int)(short)((unsigned int)(unsigned long long)g4 - (unsigned int)(int)w8)) : (unsigned short)((unsigned int)(int)g2 * (unsigned int)(short)(((long long)ga0[3] == (unsigned char)m0))));
                w8--;
            } }
            m0 = (int)f0((short)f1((long long)((!((unsigned char)g1 != (short)ga1[4])))), (short)((unsigned int)(short)f0((short)f0((short)g5, (short)ga0[3]), (short)((unsigned int)(unsigned long long)ga1[1] ^ (unsigned int)(signed char)7ull)) / ((unsigned int)(unsigned short)(~(unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(long long)g5)) | 1u)));
            { int w9 = 4; while (w9 > 0) {
                g6 = (short)((unsigned int)(unsigned long long)65535ull | (unsigned int)(unsigned short)0ull);
                w9--;
            } }
        }
    }
    for (int i10 = 0; i10 < 1; i10++) {
        m1 = (long long)((unsigned long long)(unsigned char)ga1[1] % ((unsigned long long)(unsigned short)((((unsigned int)100ull != (short)f0((short)3ull, (short)ga1[3])) && (((int)g1 == (unsigned int)ga0[2]) || ((short)g6 >= (unsigned int)g2))) ? (unsigned short)f1((long long)((unsigned long long)(unsigned int)m0 + (unsigned long long)(unsigned long long)ga0[3])) : (long long)((unsigned long long)(long long)f1((long long)g1) & (unsigned long long)(unsigned char)32767ull)) | 1u));
    }
    m2 = (unsigned int)((unsigned int)(signed char)g3 | (unsigned int)(long long)((unsigned long long)(unsigned short)g4 | (unsigned long long)(unsigned short)((((signed char)m2) || ((long long)g1 > (unsigned int)2ull)))));
    { int w11 = 3; while (w11 > 0) {
        g5 = (signed char)f0((short)(~(unsigned int)(signed char)((unsigned int)(unsigned short)7ull % ((unsigned int)(long long)1ull | 1u))), (short)100ull);
        w11--;
    } }
    for (int i12 = 0; i12 < 6; i12++) {
        switch ((int)((int)(((unsigned long long)(((int)g1 == (unsigned int)ga1[4]) ? (unsigned short)255ull : (unsigned int)255ull) < (int)((unsigned int)(long long)157ull >> ((unsigned)(unsigned int)ga0[3] & 31))) ? (signed char)((unsigned int)(long long)m0 + (unsigned int)(int)2ull) : (short)100ull)) & 7) {
        case 0:
            for (int i13 = 0; i13 < 1; i13++) {
                g1 = (signed char)(~(unsigned int)(unsigned int)m0);
            }
            break;
        case 1:
            g5 = (signed char)((unsigned int)(unsigned short)i12 & (unsigned int)(signed char)((unsigned int)(unsigned short)0ull / ((unsigned int)(unsigned int)0ull | 1u)));
        case 7:
            g2 = (short)127ull;
            break;
        default:
            { int w14 = 5; while (w14 > 0) {
                g4 = (signed char)((unsigned int)(signed char)((unsigned int)(unsigned int)f1((long long)(((!((unsigned char)65535ull)) && ((((int)255ull < (short)m1) && ((unsigned short)ga0[0] == (int)g4)) && ((((int)w14 > (int)ga0[0]) && ((unsigned short)g2 >= (unsigned int)ga1[3])) || (((unsigned char)g6 != (unsigned int)1ull) || ((signed char)ga1[1] >= (unsigned short)m1))))))) ^ (unsigned int)(int)0ull) ^ (unsigned int)(unsigned int)((unsigned int)(int)((((((short)g4 < (unsigned char)g4) || (((unsigned char)m0 != (long long)m2) || ((((unsigned int)65535ull < (int)g4) && (((long long)256ull) && ((((long long)65535ull != (int)g3) && (!(((unsigned char)g2 <= (unsigned char)g0) || ((short)0ull < (long long)g6)))) || (((unsigned short)255ull <= (unsigned long long)ga1[3]) && ((unsigned char)2ull <= (unsigned int)ga0[3]))))) && ((!(((signed char)g3 < (unsigned short)ga1[1]) || ((unsigned int)g4 != (int)100ull))) && ((!((long long)128ull <= (int)g4)) && ((short)m1 >= (int)w14)))))) && ((unsigned long long)65535ull == (signed char)7ull)) || ((unsigned char)ga0[1] < (long long)g4))) >> ((unsigned)(unsigned int)f0((short)((unsigned int)(signed char)7ull | (unsigned int)(unsigned char)65535ull), (short)((unsigned int)0 - (unsigned int)(long long)g4)) & 31)));
                w14--;
            } }
        }
    }
    g5 = (signed char)f0((short)(((short)((unsigned int)(unsigned long long)7429077594763514444ull ^ (unsigned int)(unsigned char)ga0[2]) > (short)f0((short)2ull, (short)g3)) ? (unsigned long long)((unsigned long long)(long long)g3 * (unsigned long long)(unsigned int)g5) : (unsigned short)f1((long long)g1)), (short)ga1[4]);
    switch ((int)((int)f0((short)(((unsigned int)2ull > (unsigned short)127ull)), (short)g4)) & 7) {
    case 1:
        if ((int)f0((short)g4, (short)((unsigned int)(unsigned char)ga0[2] | (unsigned int)(unsigned long long)15ull)) <= (int)((unsigned int)(long long)(((unsigned short)g3 <= (signed char)128ull) ? (unsigned char)g2 : (unsigned short)1ull) >> ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(short)g0) & 31))) {
            g4 = (signed char)((unsigned int)(int)g3 >> ((unsigned)(unsigned int)(((((short)ga1[0] <= (short)44638ull) && ((long long)2ull <= (unsigned long long)m1)) || ((unsigned char)f1((long long)m2) == (unsigned int)((unsigned int)(unsigned char)m2 * (unsigned int)(unsigned int)ga0[0])))) & 31));
        }
    case 3:
        m0 = (int)((unsigned int)(unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)0 - (unsigned long long)(unsigned char)77ull) * (unsigned long long)(unsigned short)((unsigned int)(unsigned long long)32767ull << ((unsigned)(unsigned int)2ull & 31))) % ((unsigned int)(unsigned int)((unsigned int)(short)(((unsigned long long)128ull) ? (short)ga1[4] : (int)ga0[1]) << ((unsigned)(unsigned int)g4 & 31)) | 1u));
    case 5:
        switch ((int)((int)((unsigned int)0 - (unsigned int)(unsigned int)((unsigned int)(int)m0 >> ((unsigned)(unsigned int)32767ull & 31)))) & 7) {
        case 4:
            g0 = (signed char)((unsigned int)(unsigned char)((unsigned int)(int)((unsigned int)(unsigned short)ga0[0] >> ((unsigned)(unsigned int)2ull & 31)) * (unsigned int)(unsigned int)((unsigned int)(unsigned int)2ull * (unsigned int)(short)m1)) | (unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(unsigned int)((unsigned int)(int)1ull - (unsigned int)(unsigned char)g4)));
            break;
        case 5:
            { int w15 = 3; while (w15 > 0) {
                g1 = (signed char)((unsigned int)(unsigned char)((unsigned int)(unsigned int)g1 % ((unsigned int)(long long)100ull | 1u)) / ((unsigned int)(int)((unsigned int)(long long)g5 >> ((unsigned)(unsigned int)127ull & 31)) | 1u));
                w15--;
            } }
        default:
            { int w16 = 1; while (w16 > 0) {
                m0 = (int)((unsigned int)(short)w16 << ((unsigned)(unsigned int)7ull & 31));
                w16--;
            } }
        }
        break;
    case 7:
        for (int i17 = 0; i17 < 5; i17++) {
            for (int i18 = 0; i18 < 6; i18++) {
                m1 = (long long)f1((long long)ga1[0]);
                m0 = (int)(~(unsigned int)(unsigned int)((unsigned int)(int)(((short)g2 == (long long)3ull) ? (unsigned short)256ull : (unsigned char)m2) % ((unsigned int)(unsigned int)((unsigned int)(long long)g3 / ((unsigned int)(unsigned int)3ull | 1u)) | 1u)));
            }
            m0 = (int)1ull;
            if ((unsigned char)((unsigned int)(unsigned char)((unsigned int)(int)g5 / ((unsigned int)(unsigned int)128ull | 1u)) - (unsigned int)(unsigned short)((unsigned int)(unsigned int)m2 % ((unsigned int)(unsigned char)0ull | 1u))) > (long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned short)g1 ^ (unsigned int)(unsigned char)m0) | (unsigned long long)(short)(~(unsigned int)(unsigned int)g1))) {
                g1 = (signed char)ga0[3];
                m1 = (long long)((unsigned long long)(unsigned short)m1 | (unsigned long long)(short)100ull);
                g0 = (signed char)((unsigned int)(int)((unsigned int)(long long)213ull | (unsigned int)(int)i17) % ((unsigned int)(unsigned char)((unsigned int)(int)100ull | (unsigned int)(long long)g2) | 1u));
            } else {
                m1 = (long long)(((long long)((unsigned long long)(unsigned short)((unsigned int)(short)15ull << ((unsigned)(unsigned int)127ull & 31)) - (unsigned long long)(signed char)((((unsigned int)g0 <= (short)g0) || ((unsigned short)g6 < (unsigned long long)g5)) ? (unsigned short)1ull : (unsigned long long)256ull)) == (int)65535ull));
                ga1[1] = (unsigned char)m0;
            }
        }
        break;
    default:
        g6 = (short)((unsigned int)(unsigned int)(((unsigned short)g3 > (unsigned int)(~(unsigned int)(short)ga1[1]))) | (unsigned int)(short)((unsigned int)(int)g5 | (unsigned int)(int)g5));
    }
    switch ((int)((int)3967073938ull) & 7) {
    case 0:
        if (((short)((unsigned int)(long long)g3 ^ (unsigned int)(unsigned int)ga0[1]) <= (signed char)((unsigned int)(unsigned int)256ull * (unsigned int)(unsigned int)g1)) && ((long long)1ull)) {
            switch ((int)((int)((unsigned int)(unsigned char)((unsigned int)(unsigned int)m1 >> ((unsigned)(unsigned int)255ull & 31)) * (unsigned int)(long long)(~(unsigned long long)(signed char)m2))) & 7) {
            case 3:
                ga1[0] = (unsigned char)((unsigned int)(unsigned short)f1((long long)ga1[0]) + (unsigned int)(long long)((unsigned long long)(long long)f1((long long)m2) % ((unsigned long long)(unsigned long long)(~(unsigned long long)(short)g5) | 1u)));
                break;
            case 4:
                g0 = (signed char)ga0[3];
                break;
            default:
                g1 = (signed char)((unsigned int)0 - (unsigned int)(signed char)m0);
            }
        }
        break;
    case 7:
        switch ((int)((int)(~(unsigned int)(long long)((((unsigned int)256ull > (unsigned long long)15ull) && ((unsigned short)g1)) ? (short)255ull : (unsigned long long)3ull))) & 7) {
        case 0:
            for (int i19 = 0; i19 < 5; i19++) {
                g6 = (short)g3;
                m2 = (unsigned int)((unsigned int)(unsigned char)g0 % ((unsigned int)(int)f1((long long)32767ull) | 1u));
                m1 = (long long)(((unsigned short)((unsigned int)(long long)g3 | (unsigned int)(unsigned int)g3) <= (signed char)((unsigned int)(signed char)ga0[0] - (unsigned int)(signed char)g5)) ? (unsigned int)((unsigned int)(unsigned short)ga1[4] << ((unsigned)(unsigned int)65535ull & 31)) : (unsigned long long)(~(unsigned long long)(unsigned int)m2));
            }
        case 3:
            for (int i20 = 0; i20 < 5; i20++) {
                g5 = (signed char)198ull;
            }
            break;
        case 5:
            m0 = (int)g2;
            break;
        case 7:
            m1 = (long long)((unsigned long long)(unsigned char)((unsigned int)(unsigned short)g0 * (unsigned int)(short)g1) & (unsigned long long)(unsigned short)f0((short)2ull, (short)ga1[0]));
            break;
        default:
            g1 = (signed char)g6;
        }
        break;
    default:
        for (int i21 = 0; i21 < 1; i21++) {
            if ((long long)((unsigned long long)(int)((unsigned int)(int)219586573ull - (unsigned int)(unsigned short)65535ull) >> ((unsigned)(unsigned int)f0((short)m1, (short)ga1[4]) & 63)) < (unsigned short)((unsigned int)(int)(((((unsigned char)g4 <= (int)7ull) && (((unsigned char)ga1[0] != (unsigned short)ga0[0]) && ((((unsigned short)g4) || ((short)g2)) && ((short)ga1[3] < (short)m2)))) && ((((unsigned short)i21 <= (unsigned char)g5) && ((((long long)m2 == (long long)15ull) || (!((int)g4 < (unsigned short)255ull))) && ((((unsigned int)0ull != (int)g4) && (!(((long long)m2 > (short)ga0[3]) && (!(!((unsigned long long)ga0[2] > (unsigned short)256ull)))))) || (((unsigned int)0ull == (unsigned short)ga1[3]) || ((unsigned long long)15ull))))) && ((long long)m0 != (unsigned long long)g1))) ? (unsigned short)m2 : (signed char)ga1[3]) + (unsigned int)(signed char)30ull)) {
                g0 = (signed char)i21;
                g5 = (signed char)127ull;
                g6 = (short)(((long long)ga1[0] == (short)(~(unsigned int)(unsigned char)((unsigned int)(unsigned char)g6 << ((unsigned)(unsigned int)i21 & 31)))) ? (long long)((unsigned long long)(long long)((unsigned long long)(long long)ga0[0] + (unsigned long long)(unsigned int)15ull) ^ (unsigned long long)(short)((unsigned int)(short)62022ull & (unsigned int)(unsigned long long)256ull)) : (unsigned int)((unsigned int)(signed char)f1((long long)32767ull) ^ (unsigned int)(short)((unsigned int)(signed char)ga1[0] | (unsigned int)(int)1ull)));
            }
        }
    }
    switch ((int)((int)((unsigned int)(short)g6 + (unsigned int)(short)(~(unsigned int)(signed char)15ull))) & 7) {
    case 1:
        { int w22 = 1; while (w22 > 0) {
            g2 = (short)((unsigned int)(unsigned long long)((unsigned long long)(unsigned char)ga1[4] / ((unsigned long long)(unsigned int)m0 | 1u)) >> ((unsigned)(unsigned int)((((long long)255ull < (unsigned int)g1) || ((unsigned long long)g1 > (signed char)ga1[1]))) & 31));
            w22--;
        } }
    case 3:
        g5 = (signed char)((unsigned int)(unsigned int)ga1[2] * (unsigned int)(unsigned int)g3);
        break;
    case 4:
        for (int i23 = 0; i23 < 3; i23++) {
            m1 = (long long)((unsigned long long)(unsigned short)g6 - (unsigned long long)(unsigned short)g1);
            for (int i24 = 0; i24 < 5; i24++) {
                g5 = (signed char)((unsigned int)(unsigned int)(~(unsigned int)(unsigned int)g6) | (unsigned int)(signed char)((((unsigned char)15ull > (int)m0) && (!(((unsigned long long)g2 == (short)7ull) && ((unsigned long long)g5 > (short)256ull))))));
                ga1[0] = (unsigned char)f1((long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned long long)g6 - (unsigned int)(unsigned long long)i24) >> ((unsigned)(unsigned int)((unsigned int)(unsigned int)ga0[0] & (unsigned int)(unsigned short)ga0[1]) & 63)));
            }
        }
        break;
    case 5:
        g3 = (unsigned short)((unsigned int)(long long)g1 * (unsigned int)(int)1ull);
        break;
    default:
        { int w25 = 1; while (w25 > 0) {
            g4 = (signed char)(~(unsigned int)(long long)m0);
            w25--;
        } }
    }
    if ((unsigned char)1ull > (unsigned int)f0((short)(((unsigned int)g3 == (signed char)1ull) ? (unsigned char)g3 : (unsigned char)128ull), (short)((unsigned int)(unsigned long long)3ull >> ((unsigned)(unsigned int)g6 & 31)))) {
        m2 = (unsigned int)((unsigned int)(unsigned short)((unsigned int)0 - (unsigned int)(unsigned long long)f1((long long)(~(unsigned long long)(unsigned char)128ull))) | (unsigned int)(signed char)((unsigned int)(unsigned char)(((((unsigned long long)ga1[1] != (short)g4) || ((unsigned int)100ull)) || ((long long)255ull > (unsigned long long)g1))) | (unsigned int)(unsigned short)((unsigned int)(signed char)((unsigned int)(signed char)ga1[4] | (unsigned int)(unsigned short)256ull) | (unsigned int)(short)((unsigned int)(unsigned char)m0 & (unsigned int)(unsigned int)g4))));
        g3 = (unsigned short)(((unsigned short)ga0[2] <= (signed char)((unsigned int)(unsigned int)f0((short)g1, (short)g2) << ((unsigned)(unsigned int)15ull & 31))));
    } else {
        for (int i26 = 0; i26 < 2; i26++) {
            { int w27 = 4; while (w27 > 0) {
                g1 = (signed char)((unsigned int)(short)((unsigned int)(unsigned char)(~(unsigned int)(unsigned char)(((short)g4 < (long long)g4))) / ((unsigned int)(unsigned int)((unsigned int)(short)7ull << ((unsigned)(unsigned int)3ull & 31)) | 1u)) >> ((unsigned)(unsigned int)((unsigned int)(short)(((short)(((short)ga0[3] < (unsigned short)w27)) == (long long)((unsigned long long)0 - (unsigned long long)(unsigned char)3ull)) ? (unsigned short)((unsigned int)(long long)ga1[2] / ((unsigned int)(short)3ull | 1u)) : (unsigned int)(~(unsigned int)(unsigned char)g4)) / ((unsigned int)(signed char)((unsigned int)(unsigned char)(((unsigned long long)255ull > (unsigned int)g1) ? (unsigned short)g2 : (short)g5) * (unsigned int)(long long)(~(unsigned long long)(unsigned long long)ga1[4])) | 1u)) & 31));
                w27--;
            } }
        }
    }
    for (int i28 = 0; i28 < 3; i28++) {
        switch ((int)((int)((unsigned int)(signed char)((unsigned int)(unsigned short)g5 | (unsigned int)(signed char)ga1[4]) + (unsigned int)(unsigned short)g4)) & 7) {
        case 0:
            switch ((int)((int)((unsigned int)(unsigned short)((unsigned int)(unsigned short)255ull >> ((unsigned)(unsigned int)255ull & 31)) % ((unsigned int)(unsigned short)((unsigned int)(unsigned char)127ull << ((unsigned)(unsigned int)ga1[0] & 31)) | 1u))) & 7) {
            case 1:
                m2 = (unsigned int)f1((long long)((unsigned long long)(unsigned long long)((unsigned long long)(int)g1 & (unsigned long long)(unsigned int)g1) / ((unsigned long long)(int)((((((unsigned int)0ull != (short)ga1[2]) || ((signed char)65535ull)) && ((unsigned char)g4 >= (signed char)3ull)) && (((signed char)g1 >= (int)255ull) || (!(((unsigned long long)2ull != (unsigned short)ga0[2]) || (((unsigned long long)256ull <= (int)g1) && ((long long)g1 >= (int)g6))))))) | 1u)));
                break;
            case 2:
                g5 = (signed char)128ull;
            default:
                g2 = (short)((unsigned int)(int)((unsigned int)0 - (unsigned int)(unsigned int)ga1[1]) + (unsigned int)(unsigned short)((((unsigned int)m0 <= (signed char)g6) || (((int)256ull < (long long)ga0[3]) || ((unsigned char)7ull < (unsigned char)g5)))));
            }
            break;
        case 2:
            m2 = (unsigned int)(~(unsigned int)(unsigned long long)((unsigned long long)(unsigned char)g0 * (unsigned long long)(unsigned long long)((unsigned long long)(unsigned long long)(((unsigned char)2ull >= (signed char)g6) ? (long long)0ull : (unsigned char)100ull) - (unsigned long long)(unsigned long long)((unsigned long long)0 - (unsigned long long)(long long)i28))));
            break;
        case 3:
            g0 = (signed char)((unsigned int)(long long)((unsigned long long)(unsigned short)m1 >> ((unsigned)(unsigned int)m0 & 63)) ^ (unsigned int)(signed char)((unsigned int)0 - (unsigned int)(short)ga0[3]));
            break;
        case 4:
            for (int i29 = 0; i29 < 5; i29++) {
                m1 = (long long)((unsigned long long)(signed char)((unsigned int)0 - (unsigned int)(unsigned long long)((unsigned long long)(long long)((unsigned long long)(unsigned int)256ull << ((unsigned)(unsigned int)32767ull & 63)) + (unsigned long long)(signed char)f0((short)32767ull, (short)65535ull))) >> ((unsigned)(unsigned int)((unsigned int)(unsigned short)((unsigned int)(short)((unsigned int)(unsigned int)ga0[2] & (unsigned int)(long long)65535ull) * (unsigned int)(unsigned short)((unsigned int)(unsigned long long)m0 ^ (unsigned int)(short)ga0[0])) + (unsigned int)(short)((!((((int)128ull >= (unsigned short)g0) && ((long long)g1)) && (((signed char)g0 >= (int)ga1[0]) && (((((unsigned int)ga1[4] <= (unsigned char)ga0[3]) && ((!((((signed char)m1 <= (short)ga1[1]) && ((unsigned long long)255ull > (signed char)100ull)) || ((signed char)255ull <= (unsigned char)52ull))) && (!((signed char)ga0[3] > (unsigned int)i28)))) || ((unsigned short)ga0[0])) && ((short)256ull <= (long long)ga0[3]))))) ? (long long)((unsigned long long)(long long)g4 - (unsigned long long)(unsigned short)3ull) : (int)((unsigned int)(unsigned long long)ga1[1] >> ((unsigned)(unsigned int)ga0[3] & 31)))) & 63));
            }
            break;
        default:
            if ((unsigned short)((unsigned int)(signed char)ga1[3] - (unsigned int)(signed char)((unsigned int)(short)255ull - (unsigned int)(signed char)ga1[1])) >= (unsigned long long)((unsigned long long)(short)(~(unsigned int)(unsigned long long)65535ull) ^ (unsigned long long)(int)7ull)) {
                g2 = (short)m2;
                m2 = (unsigned int)((unsigned int)0 - (unsigned int)(signed char)((unsigned int)0 - (unsigned int)(unsigned char)((unsigned int)(short)2ull - (unsigned int)(int)ga0[1])));
                g6 = (short)(~(unsigned int)(unsigned short)1ull);
            } else {
                g3 = (unsigned short)((unsigned int)(unsigned char)((unsigned int)(unsigned short)m0 << ((unsigned)(unsigned int)g6 & 31)) << ((unsigned)(unsigned int)((unsigned int)(short)ga1[3] | (unsigned int)(unsigned long long)32767ull) & 31));
            }
        }
        g2 = (short)((((long long)f1((long long)ga0[1]) <= (long long)((unsigned long long)(signed char)ga0[0] ^ (unsigned long long)(unsigned int)((((long long)0ull == (short)1ull) || ((unsigned short)g6))))) || ((int)((unsigned int)(unsigned char)32767ull ^ (unsigned int)(unsigned char)((unsigned int)(unsigned long long)335075747250790428ull * (unsigned int)(short)i28)) < (long long)((unsigned long long)(int)(((long long)15ull < (long long)g2)) * (unsigned long long)(unsigned short)((unsigned int)(unsigned short)65535ull * (unsigned int)(int)7ull)))) ? (unsigned char)(((signed char)f1((long long)f1((long long)ga0[2])) != (unsigned int)m1) ? (unsigned long long)((unsigned long long)(long long)(~(unsigned long long)(short)ga1[1]) * (unsigned long long)(unsigned char)((unsigned int)(int)ga0[3] * (unsigned int)(long long)255ull)) : (unsigned int)((unsigned int)(unsigned long long)((unsigned long long)(unsigned long long)g1 - (unsigned long long)(signed char)g5) << ((unsigned)(unsigned int)g5 & 31))) : (unsigned char)((unsigned int)(long long)((unsigned long long)(signed char)(((((((long long)g4 >= (short)g5) || (((unsigned short)g2) && ((unsigned int)ga0[2] >= (int)g6))) || ((unsigned long long)g1 >= (unsigned char)g1)) && ((unsigned int)g2 != (unsigned short)166ull)) && (((int)ga0[2]) && (((int)3ull < (signed char)0ull) && (((unsigned long long)g5 == (unsigned short)32767ull) || ((unsigned int)32767ull >= (unsigned short)3ull)))))) + (unsigned long long)(signed char)(~(unsigned int)(int)32767ull)) ^ (unsigned int)(long long)((unsigned long long)(unsigned int)(~(unsigned int)(int)g3) + (unsigned long long)(unsigned long long)((unsigned long long)(unsigned int)ga0[2] - (unsigned long long)(unsigned int)m2))));
    }
    switch ((int)((int)((unsigned int)(signed char)3ull % ((unsigned int)(signed char)((unsigned int)(unsigned short)m0 / ((unsigned int)(signed char)g5 | 1u)) | 1u))) & 7) {
    case 1:
        for (int i30 = 0; i30 < 3; i30++) {
            m2 = (unsigned int)f1((long long)f1((long long)((unsigned long long)(signed char)ga0[2] * (unsigned long long)(int)g1)));
            ga1[3] = (unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(int)0ull % ((unsigned long long)(signed char)((unsigned int)(int)255ull | (unsigned int)(short)g0) | 1u)) >> ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(int)((unsigned int)(signed char)m1 | (unsigned int)(long long)65535ull)) & 31));
            m2 = (unsigned int)(((short)15ull != (long long)g5) ? (signed char)256ull : (signed char)g0);
        }
        break;
    case 2:
        for (int i31 = 0; i31 < 3; i31++) {
            ga1[3] = (unsigned char)((unsigned int)(int)(((unsigned char)((((short)g0 == (unsigned long long)32767ull) && ((unsigned int)g6 >= (long long)g0)) ? (unsigned short)g5 : (unsigned short)55137ull)) ? (short)((unsigned int)(long long)g6 ^ (unsigned int)(short)7ull) : (int)f0((short)0ull, (short)g1)) << ((unsigned)(unsigned int)((unsigned int)(unsigned int)(((((!((unsigned short)ga1[0] <= (signed char)m2)) || ((unsigned int)m0)) && ((unsigned long long)2ull > (unsigned long long)256ull)) || ((unsigned char)127ull > (unsigned short)0ull)) ? (signed char)ga0[2] : (short)ga1[0]) % ((unsigned int)(unsigned int)(((!((unsigned short)7ull > (signed char)15ull)) || ((((long long)ga0[2] < (unsigned char)m1) || ((long long)15ull == (int)g5)) && ((!((!(((long long)g0) && ((int)128ull < (short)ga1[1]))) && (!((unsigned char)i31 <= (unsigned int)g5)))) || (((((unsigned int)g0 > (unsigned char)65535ull) || ((((unsigned short)3ull > (unsigned short)i31) || (!((short)g2 <= (short)m0))) && (((unsigned char)ga1[3] < (unsigned int)g1) && (((short)g0 >= (unsigned long long)0ull) && ((unsigned long long)g3 > (unsigned int)ga0[1]))))) || ((int)2ull <= (int)ga1[2])) || (((int)3ull == (int)ga0[3]) && ((((unsigned short)g0 < (long long)i31) && ((long long)ga1[4] == (unsigned long long)255ull)) && ((unsigned char)g6 == (unsigned char)7ull)))))))) | 1u)) & 31));
            m0 = (int)((unsigned int)(unsigned char)f0((short)((unsigned int)(long long)((unsigned long long)(unsigned char)i31 - (unsigned long long)(unsigned short)g1) + (unsigned int)(unsigned int)f1((long long)i31)), (short)((unsigned int)(unsigned char)((unsigned int)(unsigned char)ga1[2] % ((unsigned int)(signed char)65535ull | 1u)) / ((unsigned int)(long long)((unsigned long long)(unsigned char)g3 / ((unsigned long long)(unsigned int)73ull | 1u)) | 1u))) % ((unsigned int)(signed char)((unsigned int)(unsigned long long)g6 & (unsigned int)(unsigned long long)(((unsigned int)2ull < (unsigned int)((unsigned int)(unsigned char)7ull - (unsigned int)(long long)255ull)))) | 1u));
        }
        break;
    case 3:
        { int w32 = 3; while (w32 > 0) {
            if (((short)((((short)ga0[0] != (unsigned char)g0) && (((unsigned short)g6 > (unsigned int)100ull) && (((short)g5 < (unsigned short)g3) && ((unsigned char)100ull >= (unsigned short)65535ull))))) != (unsigned char)(~(unsigned int)(unsigned long long)237ull)) && ((((unsigned short)g2 <= (unsigned char)m2) || (((((unsigned char)ga1[0] != (short)ga0[1]) || ((short)0ull >= (signed char)255ull)) && (((short)m0) && (!((unsigned short)0ull <= (signed char)255ull)))) && (((signed char)127ull) && (((int)7ull < (int)g0) && (((unsigned int)g4 < (long long)m2) || ((short)g2 > (signed char)175ull)))))) || ((unsigned char)g4 <= (signed char)m2))) {
                m2 = (unsigned int)((unsigned int)(unsigned short)f1((long long)((unsigned long long)(int)ga0[2] / ((unsigned long long)(unsigned char)g4 | 1u))) | (unsigned int)(long long)((unsigned long long)(unsigned char)(~(unsigned int)(unsigned char)g1) / ((unsigned long long)(unsigned char)(~(unsigned int)(long long)g3) | 1u)));
                g2 = (short)f0((short)((unsigned int)0 - (unsigned int)(unsigned long long)ga0[0]), (short)f1((long long)65535ull));
                m1 = (long long)255ull;
            }
            w32--;
        } }
    case 7:
        g6 = (short)(((int)((unsigned int)(short)m2 ^ (unsigned int)(short)256ull) != (unsigned char)g3));
    default:
        for (int i33 = 0; i33 < 5; i33++) {
            m0 = (int)(((unsigned long long)g2 < (unsigned long long)m2) ? (unsigned long long)32767ull : (short)35ull);
        }
    }
    switch ((int)((int)g1) & 7) {
    case 1:
        ga0[0] = (unsigned int)ga1[2];
        break;
    case 3:
        ga0[0] = (unsigned int)((unsigned int)(unsigned short)((unsigned int)(int)((unsigned int)(int)g0 - (unsigned int)(unsigned char)g0) - (unsigned int)(unsigned int)f1((long long)ga0[1])) + (unsigned int)(unsigned long long)(~(unsigned long long)(unsigned short)(~(unsigned int)(long long)g1)));
        break;
    default:
        if ((unsigned char)((unsigned int)(signed char)m2 / ((unsigned int)(unsigned short)((unsigned int)(int)ga1[0] * (unsigned int)(long long)g2) | 1u)) <= (short)(~(unsigned int)(unsigned long long)((unsigned long long)(unsigned char)ga0[2] + (unsigned long long)(signed char)ga1[4]))) {
            m1 = (long long)f0((short)f0((short)g2, (short)(((!((unsigned int)15ull <= (unsigned short)m2)) || ((unsigned char)g5)))), (short)ga0[0]);
            g5 = (signed char)((unsigned int)(unsigned char)(((int)ga1[2] > (int)g6) ? (long long)g3 : (unsigned int)g4) & (unsigned int)(unsigned short)((unsigned int)(unsigned short)ga1[4] * (unsigned int)(long long)m2));
        } else {
            if ((unsigned int)((unsigned int)(unsigned char)((unsigned int)(signed char)32767ull >> ((unsigned)(unsigned int)2ull & 31)) - (unsigned int)(long long)m2) >= (unsigned char)(((unsigned long long)g4 >= (long long)127ull) ? (unsigned long long)((unsigned long long)(short)ga1[2] & (unsigned long long)(unsigned char)65535ull) : (signed char)((unsigned int)(unsigned long long)g6 | (unsigned int)(unsigned char)65535ull))) {
                g5 = (signed char)m2;
                g0 = (signed char)((unsigned int)(unsigned long long)f1((long long)(((long long)((unsigned long long)(signed char)255ull >> ((unsigned)(unsigned int)g5 & 63)) != (long long)(~(unsigned long long)(long long)g1)) ? (short)((unsigned int)(signed char)ga1[0] / ((unsigned int)(unsigned long long)32767ull | 1u)) : (signed char)((unsigned int)(long long)g1 % ((unsigned int)(unsigned long long)127ull | 1u)))) & (unsigned int)(unsigned short)127ull);
            } else {
                ga0[0] = (unsigned int)((unsigned int)(signed char)m1 >> ((unsigned)(unsigned int)(~(unsigned int)(unsigned char)11ull) & 31));
            }
            g4 = (signed char)f0((short)(((long long)127ull) ? (unsigned long long)g2 : (unsigned short)g5), (short)((unsigned int)0 - (unsigned int)(unsigned int)((unsigned int)(unsigned long long)3ull ^ (unsigned int)(unsigned int)ga0[2])));
        }
    }
    switch ((int)((int)m2) & 7) {
    case 0:
        m0 = (int)(~(unsigned int)(unsigned long long)65535ull);
    case 2:
        ga1[1] = (unsigned char)m1;
        break;
    case 4:
        for (int i34 = 0; i34 < 3; i34++) {
            g1 = (signed char)((unsigned int)(unsigned long long)ga0[0] + (unsigned int)(unsigned long long)g4);
        }
        break;
    case 7:
        m0 = (int)((unsigned int)(unsigned char)((unsigned int)(unsigned int)((unsigned int)(int)((unsigned int)(unsigned long long)65535ull & (unsigned int)(long long)g1) / ((unsigned int)(int)((unsigned int)(long long)ga0[0] ^ (unsigned int)(unsigned long long)ga0[3]) | 1u)) * (unsigned int)(unsigned char)((unsigned int)(unsigned long long)g3 * (unsigned int)(unsigned long long)((unsigned long long)(short)g1 ^ (unsigned long long)(unsigned int)1ull))) / ((unsigned int)(unsigned char)((unsigned int)(unsigned int)(((short)((unsigned int)(short)g1 ^ (unsigned int)(signed char)ga1[2]) >= (unsigned short)((unsigned int)(short)ga1[4] ^ (unsigned int)(signed char)15ull))) >> ((unsigned)(unsigned int)((unsigned int)(signed char)((unsigned int)(short)ga1[1] % ((unsigned int)(int)ga1[0] | 1u)) & (unsigned int)(signed char)(((int)65535ull > (unsigned short)127ull) ? (long long)g6 : (signed char)g2)) & 31)) | 1u));
        break;
    default:
        g2 = (short)100ull;
    }
    runtime.printf("g0=%llx\n", (unsigned long long)g0);
    runtime.printf("g1=%llx\n", (unsigned long long)g1);
    runtime.printf("g2=%llx\n", (unsigned long long)g2);
    runtime.printf("g3=%llx\n", (unsigned long long)g3);
    runtime.printf("g4=%llx\n", (unsigned long long)g4);
    runtime.printf("g5=%llx\n", (unsigned long long)g5);
    runtime.printf("g6=%llx\n", (unsigned long long)g6);
    runtime.printf("m0=%llx\n", (unsigned long long)m0);
    runtime.printf("m1=%llx\n", (unsigned long long)m1);
    runtime.printf("m2=%llx\n", (unsigned long long)m2);
    runtime.printf("ga0_0=%llx\n", (unsigned long long)ga0[0]);
    runtime.printf("ga0_1=%llx\n", (unsigned long long)ga0[1]);
    runtime.printf("ga0_2=%llx\n", (unsigned long long)ga0[2]);
    runtime.printf("ga0_3=%llx\n", (unsigned long long)ga0[3]);
    runtime.printf("ga1_0=%llx\n", (unsigned long long)ga1[0]);
    runtime.printf("ga1_1=%llx\n", (unsigned long long)ga1[1]);
    runtime.printf("ga1_2=%llx\n", (unsigned long long)ga1[2]);
    runtime.printf("ga1_3=%llx\n", (unsigned long long)ga1[3]);
    runtime.printf("ga1_4=%llx\n", (unsigned long long)ga1[4]);
    return 0;
}

