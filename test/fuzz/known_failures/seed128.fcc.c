package main;
import runtime;
unsigned short g0 = (unsigned short)32767ull;
long long g1 = (long long)2ull;
unsigned int g2 = (unsigned int)128ull;
short g3 = (short)60169ull;
unsigned long long g4 = (unsigned long long)15ull;
signed char g5 = (signed char)100ull;
short g6 = (short)44460ull;
unsigned int g7 = (unsigned int)50ull;
int g8 = (int)0ull;
int g9 = (int)2ull;
unsigned char g10 = (unsigned char)255ull;
unsigned char g11 = (unsigned char)127ull;
signed char ga0[5] = {(signed char)100ull, (signed char)65535ull, (signed char)15ull, (signed char)10ull, (signed char)2ull};
unsigned int ga1[3] = {(unsigned int)100ull, (unsigned int)7ull, (unsigned int)127ull};
static signed char f0(unsigned short p0, unsigned short p1, long long p2, short p3) {
    unsigned char l0 = (unsigned char)65535ull;
    int l1 = (int)(~(unsigned int)(short)((unsigned int)(unsigned short)g9 << ((unsigned)(unsigned int)847960452ull & 31)));
    for (int i0 = 0; i0 < 5; i0++) {
        p2 = (long long)(((signed char)l1 < (unsigned char)(~(unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(unsigned long long)((((!(((unsigned short)g1 <= (unsigned int)g7) && ((long long)255ull > (unsigned short)0ull))) || ((unsigned int)g1 <= (unsigned short)65293ull)) || (((((long long)65535ull != (int)g4) && (((short)2ull < (unsigned char)32767ull) || ((unsigned char)ga1[1] > (signed char)127ull))) && ((unsigned long long)128ull >= (signed char)g10)) || ((unsigned char)g4 > (long long)127ull))))))));
        if ((((signed char)ga1[1] == (unsigned long long)g0) && ((unsigned char)i0 > (unsigned long long)p1)) || ((unsigned long long)(((int)ga0[4] > (int)l1)))) {
            g6 = (short)g5;
            l0 = (unsigned char)((unsigned int)0 - (unsigned int)(short)((unsigned int)(short)1ull ^ (unsigned int)(long long)ga0[1]));
        }
        if ((unsigned short)p3 > (short)(~(unsigned int)(unsigned long long)((unsigned long long)(long long)p0 % ((unsigned long long)(unsigned char)g10 | 1u)))) {
            g2 = (unsigned int)((unsigned int)(long long)(((unsigned char)((unsigned int)0 - (unsigned int)(int)((unsigned int)(signed char)65535ull << ((unsigned)(unsigned int)256ull & 31))) < (unsigned char)128ull) ? (unsigned int)((unsigned int)(unsigned short)((unsigned int)(int)65535ull | (unsigned int)(long long)256ull) % ((unsigned int)(signed char)g1 | 1u)) : (unsigned int)(~(unsigned int)(long long)g11)) << ((unsigned)(unsigned int)(~(unsigned int)(unsigned int)(((signed char)(~(unsigned int)(signed char)g8)) ? (long long)((unsigned long long)0 - (unsigned long long)(signed char)7ull) : (signed char)((unsigned int)(unsigned long long)g6 | (unsigned int)(short)32767ull))) & 31));
            p3 = (short)g6;
            g3 = (short)((unsigned int)(unsigned long long)((unsigned long long)(short)g2 - (unsigned long long)(long long)((unsigned long long)(short)((unsigned int)(unsigned char)ga0[0] / ((unsigned int)(int)g10 | 1u)) / ((unsigned long long)(int)((unsigned int)(short)g7 + (unsigned int)(short)100ull) | 1u))) * (unsigned int)(signed char)((unsigned int)(unsigned long long)(((unsigned long long)100ull != (unsigned long long)((unsigned long long)(short)100ull << ((unsigned)(unsigned int)1ull & 63)))) / ((unsigned int)(unsigned int)g9 | 1u)));
        } else {
            g4 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(signed char)(((long long)l1) ? (unsigned char)g2 : (unsigned long long)g11) - (unsigned long long)(long long)g11) + (unsigned long long)(short)((unsigned int)(unsigned short)g2 % ((unsigned int)(signed char)((unsigned int)(unsigned long long)100ull & (unsigned int)(unsigned int)2ull) | 1u)));
            g5 = (signed char)g5;
            l0 = (unsigned char)((unsigned int)(signed char)p2 & (unsigned int)(short)ga0[3]);
        }
    }
    l1 = (int)(~(unsigned int)(unsigned int)((unsigned int)(signed char)g4 * (unsigned int)(unsigned short)0ull));
    return (signed char)((unsigned int)(unsigned int)((unsigned int)(unsigned long long)p0 | (unsigned int)(signed char)g0) & (unsigned int)(int)((((unsigned int)l1 > (unsigned char)g4) && ((int)2ull > (signed char)g2)) ? (signed char)32767ull : (signed char)2ull));
}

static long long f1(long long p0, signed char p1, unsigned long long p2, long long p3) {
    unsigned short l0 = (unsigned short)f0((unsigned short)((unsigned int)(short)g2 - (unsigned int)(unsigned int)g5), (unsigned short)f0((unsigned short)ga1[2], (unsigned short)g4, (long long)1ull, (short)ga0[4]), (long long)((unsigned long long)(unsigned char)ga0[1] << ((unsigned)(unsigned int)g10 & 63)), (short)((unsigned int)(unsigned long long)g9 * (unsigned int)(short)255ull));
    g0 = (unsigned short)((unsigned int)(unsigned char)((((long long)((unsigned long long)(unsigned short)ga1[1] ^ (unsigned long long)(unsigned long long)7ull) <= (short)((unsigned int)(unsigned char)65535ull ^ (unsigned int)(short)g8)) && (((unsigned short)2ull <= (unsigned long long)g3) && ((signed char)ga0[2] != (int)2ull))) ? (unsigned long long)((unsigned long long)(int)((unsigned int)0 - (unsigned int)(int)g11) / ((unsigned long long)(unsigned short)f0((unsigned short)65535ull, (unsigned short)ga1[0], (long long)l0, (short)g8) | 1u)) : (unsigned long long)((unsigned long long)(long long)(((((((unsigned long long)ga1[0] != (unsigned int)100ull) || ((long long)ga0[3] != (int)ga1[1])) && ((short)g9 <= (long long)l0)) && (!((((unsigned char)p2) && ((unsigned char)ga1[2] <= (signed char)7ull)) && ((unsigned int)p2 <= (unsigned short)p3)))) && (((signed char)g10 != (unsigned char)g8) || (((int)2ull <= (int)ga1[1]) || ((unsigned short)1ull != (int)g7))))) << ((unsigned)(unsigned int)((unsigned int)(signed char)g7 & (unsigned int)(unsigned int)ga1[0]) & 63))) / ((unsigned int)(int)g10 | 1u));
    for (int i0 = 0; i0 < 2; i0++) {
        p1 = (signed char)((unsigned int)(unsigned short)((unsigned int)(unsigned short)((unsigned int)(unsigned short)256ull - (unsigned int)(long long)2ull) ^ (unsigned int)(unsigned char)((unsigned int)(long long)7125599880076427038ull + (unsigned int)(short)127ull)) >> ((unsigned)(unsigned int)(~(unsigned int)(signed char)ga1[1]) & 31));
    }
    switch ((int)((int)((unsigned int)(short)((unsigned int)(unsigned int)15ull >> ((unsigned)(unsigned int)g2 & 31)) * (unsigned int)(unsigned long long)(~(unsigned long long)(unsigned short)2ull))) & 7) {
    case 0:
        switch ((int)((int)(((long long)f0((unsigned short)15ull, (unsigned short)g8, (long long)255ull, (short)32767ull) < (unsigned long long)3ull))) & 7) {
        case 0:
            p2 = (unsigned long long)((unsigned long long)(int)ga1[1] * (unsigned long long)(unsigned long long)127ull);
            break;
        case 3:
            g7 = (unsigned int)g5;
            break;
        default:
            p3 = (long long)((unsigned long long)(short)((unsigned int)(unsigned char)p3 / ((unsigned int)(long long)1ull | 1u)) / ((unsigned long long)(unsigned int)(((int)255ull)) | 1u));
        }
    case 2:
        p2 = (unsigned long long)((unsigned long long)(signed char)255ull / ((unsigned long long)(int)p0 | 1u));
        break;
    default:
        for (int i1 = 0; i1 < 5; i1++) {
            g3 = (short)(((short)(~(unsigned int)(long long)((unsigned long long)(unsigned short)127ull - (unsigned long long)(unsigned int)3ull))) ? (signed char)((unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(long long)65535ull) | (unsigned int)(unsigned long long)((unsigned long long)(int)7ull & (unsigned long long)(unsigned int)32767ull)) : (short)ga1[0]);
            g2 = (unsigned int)((unsigned int)(signed char)((unsigned int)(unsigned short)p2 + (unsigned int)(signed char)256ull) + (unsigned int)(short)((unsigned int)(unsigned short)7ull & (unsigned int)(int)p1));
        }
    }
    for (int i2 = 0; i2 < 3; i2++) {
        if (((unsigned short)(((int)p1 != (unsigned short)127ull) ? (unsigned int)ga0[0] : (int)g4) > (signed char)((unsigned int)(short)32767ull << ((unsigned)(unsigned int)g9 & 31))) && ((long long)32767ull > (int)((unsigned int)(long long)p3 * (unsigned int)(unsigned short)p1))) {
            g11 = (unsigned char)g11;
        }
        for (int i3 = 0; i3 < 1; i3++) {
            g0 = (unsigned short)f0((unsigned short)((unsigned int)(signed char)((unsigned int)(long long)i2 % ((unsigned int)(short)ga1[0] | 1u)) & (unsigned int)(int)(((unsigned short)ga0[0] == (unsigned int)ga0[1]))), (unsigned short)(((unsigned char)((unsigned int)(unsigned int)255ull & (unsigned int)(int)ga1[0]))), (long long)((unsigned long long)0 - (unsigned long long)(signed char)((unsigned int)(unsigned long long)p0 << ((unsigned)(unsigned int)32767ull & 31))), (short)p2);
        }
        g2 = (unsigned int)(((((short)1ull > (int)g3) || ((unsigned long long)g8 >= (unsigned short)p3)) || ((unsigned short)15ull > (unsigned short)2ull)));
    }
    g4 = (unsigned long long)((unsigned long long)(signed char)((unsigned int)(long long)p2 & (unsigned int)(unsigned long long)ga1[2]) >> ((unsigned)(unsigned int)((unsigned int)(unsigned short)p1 ^ (unsigned int)(unsigned char)p0) & 63));
    if ((!((((short)0ull > (int)100ull) || ((signed char)255ull < (unsigned short)0ull)) && ((long long)ga1[1] == (unsigned long long)g7))) && ((unsigned long long)(((unsigned short)ga1[0]) ? (int)ga0[2] : (signed char)256ull) >= (unsigned short)2ull)) {
        p2 = (unsigned long long)((unsigned long long)(unsigned char)100ull - (unsigned long long)(unsigned long long)10341162414805772545ull);
        p1 = (signed char)((unsigned int)(short)(~(unsigned int)(short)21911ull) | (unsigned int)(short)((unsigned int)(short)65535ull & (unsigned int)(unsigned char)l0));
        ga0[2] = (signed char)((unsigned int)(short)((unsigned int)(long long)f0((unsigned short)1ull, (unsigned short)ga0[3], (long long)15ull, (short)127ull) + (unsigned int)(signed char)((unsigned int)(unsigned long long)0ull << ((unsigned)(unsigned int)256ull & 31))) << ((unsigned)(unsigned int)ga0[1] & 31));
    } else {
        for (int i4 = 0; i4 < 6; i4++) {
            p3 = (long long)((unsigned long long)(unsigned short)(((signed char)((unsigned int)(unsigned int)ga1[1] - (unsigned int)(long long)p0) < (signed char)((unsigned int)(unsigned short)f0((unsigned short)g2, (unsigned short)g0, (long long)255ull, (short)g0) << ((unsigned)(unsigned int)f0((unsigned short)g2, (unsigned short)p3, (long long)32767ull, (short)g5) & 31)))) ^ (unsigned long long)(short)((((long long)f0((unsigned short)96ull, (unsigned short)p0, (long long)g3, (short)ga0[2]) > (int)((unsigned int)(unsigned long long)3ull & (unsigned int)(unsigned long long)p3)) && (((unsigned long long)g10 == (unsigned short)15ull) && ((unsigned short)61ull != (signed char)13ull))) ? (unsigned char)f0((unsigned short)((unsigned int)(unsigned long long)1ull ^ (unsigned int)(unsigned short)g11), (unsigned short)ga1[1], (long long)((unsigned long long)(unsigned int)p2 / ((unsigned long long)(unsigned char)g4 | 1u)), (short)((unsigned int)(unsigned short)ga0[2] * (unsigned int)(signed char)161ull)) : (unsigned char)((((short)255ull) || ((unsigned char)ga0[2] >= (short)g8)))));
            g5 = (signed char)((unsigned int)(int)((((unsigned short)g8) || ((((long long)i4 != (int)g4) || (((signed char)g9) && ((unsigned long long)ga1[0] < (unsigned int)ga0[1]))) || (((((unsigned char)g8 >= (unsigned char)3ull) && ((((int)ga0[3] > (signed char)ga0[0]) || ((unsigned long long)p0 > (unsigned int)ga0[1])) || ((int)7ull))) && ((signed char)p1)) || (((unsigned long long)ga1[2] != (unsigned long long)p0) && ((int)ga1[0])))))) << ((unsigned)(unsigned int)ga1[2] & 31));
        }
        for (int i5 = 0; i5 < 3; i5++) {
            g0 = (unsigned short)((unsigned int)(int)((unsigned int)(long long)((unsigned long long)(unsigned long long)(~(unsigned long long)(long long)p0) ^ (unsigned long long)(unsigned short)((unsigned int)(signed char)g1 + (unsigned int)(unsigned short)ga0[3])) >> ((unsigned)(unsigned int)g3 & 31)) + (unsigned int)(unsigned short)f0((unsigned short)((unsigned int)(unsigned long long)((unsigned long long)(int)ga0[3] / ((unsigned long long)(signed char)255ull | 1u)) | (unsigned int)(signed char)((unsigned int)(int)32767ull & (unsigned int)(short)255ull)), (unsigned short)((unsigned int)0 - (unsigned int)(unsigned long long)ga1[2]), (long long)((unsigned long long)(unsigned int)((((unsigned long long)g1 >= (short)g1) && ((!((unsigned int)ga0[2] != (unsigned int)p1)) || ((unsigned short)15ull >= (unsigned short)g4))) ? (short)65535ull : (unsigned char)g3) >> ((unsigned)(unsigned int)((unsigned int)(short)ga0[0] % ((unsigned int)(unsigned int)ga1[2] | 1u)) & 63)), (short)f0((unsigned short)((unsigned int)(unsigned int)g9 | (unsigned int)(unsigned char)ga0[1]), (unsigned short)f0((unsigned short)7ull, (unsigned short)l0, (long long)ga1[2], (short)128ull), (long long)((unsigned long long)(unsigned long long)11ull ^ (unsigned long long)(unsigned long long)7ull), (short)((unsigned int)(unsigned short)127ull | (unsigned int)(int)l0))));
            ga0[1] = (signed char)g0;
            g8 = (int)p2;
        }
    }
    return (long long)(((short)((unsigned int)(unsigned long long)0ull / ((unsigned int)(short)128ull | 1u)) <= (unsigned long long)((unsigned long long)(long long)255ull << ((unsigned)(unsigned int)g10 & 63))));
}

static signed char f2(unsigned long long p0, unsigned long long p1) {
    unsigned char l0 = (unsigned char)((unsigned int)(signed char)((unsigned int)0 - (unsigned int)(unsigned long long)ga1[2]) * (unsigned int)(unsigned long long)109ull);
    if ((unsigned long long)(((signed char)(((signed char)g5 > (unsigned char)g6) ? (int)128ull : (signed char)202ull) == (unsigned short)ga0[2]) ? (unsigned long long)((unsigned long long)(unsigned long long)65535ull | (unsigned long long)(unsigned int)ga1[1]) : (unsigned int)238ull) >= (unsigned short)f0((unsigned short)(((unsigned long long)32767ull != (unsigned short)g1)), (unsigned short)((((unsigned long long)ga0[0] >= (unsigned char)p0) || (!(((unsigned long long)g8 <= (unsigned char)l0) || ((long long)g7 >= (short)g0)))) ? (unsigned long long)ga1[1] : (short)100ull), (long long)((unsigned long long)(unsigned char)p1 * (unsigned long long)(short)3ull), (short)(((short)g11 != (unsigned int)ga0[3]) ? (unsigned char)ga0[3] : (signed char)32767ull))) {
        g10 = (unsigned char)(~(unsigned int)(unsigned short)256ull);
    }
    { int w0 = 1; while (w0 > 0) {
        g9 = (int)((unsigned int)(int)g4 % ((unsigned int)(short)l0 | 1u));
        w0--;
    } }
    if ((unsigned long long)(~(unsigned long long)(long long)f0((unsigned short)l0, (unsigned short)g3, (long long)g8, (short)ga1[2]))) {
        g6 = (short)f0((unsigned short)(((((short)g0) && (((unsigned int)g9) && ((unsigned char)191ull == (signed char)g10))) && ((((unsigned char)255ull <= (unsigned short)g7) || (!((short)ga0[3]))) && ((int)ga1[0] <= (int)15ull))) ? (unsigned int)f0((unsigned short)g0, (unsigned short)65535ull, (long long)p0, (short)ga1[1]) : (unsigned int)f1((long long)7ull, (signed char)148ull, (unsigned long long)2ull, (long long)ga1[2])), (unsigned short)7ull, (long long)((unsigned long long)0 - (unsigned long long)(signed char)(~(unsigned int)(unsigned char)128ull)), (short)((unsigned int)0 - (unsigned int)(int)((((unsigned char)g10 != (signed char)g2) || ((unsigned long long)ga0[3] != (short)1ull)) ? (unsigned short)32767ull : (unsigned long long)15ull)));
        { int w1 = 2; while (w1 > 0) {
            g6 = (short)p1;
            w1--;
        } }
    }
    switch ((int)((int)((((unsigned short)128ull < (unsigned char)g7) && (((unsigned int)256ull != (unsigned int)ga1[1]) && ((unsigned int)ga1[2] > (unsigned long long)4988474985099290884ull))) ? (signed char)((unsigned int)(signed char)g1 & (unsigned int)(unsigned char)g4) : (signed char)(~(unsigned int)(int)7ull))) & 7) {
    case 1:
        for (int i2 = 0; i2 < 4; i2++) {
            g4 = (unsigned long long)(~(unsigned long long)(unsigned long long)g9);
        }
    case 2:
        { int w3 = 2; while (w3 > 0) {
            g8 = (int)((unsigned int)(unsigned int)((!((((unsigned long long)ga0[3]) && (!(((unsigned char)ga0[0] >= (unsigned short)ga0[3]) || ((unsigned int)ga1[2])))) || (((unsigned long long)7ull <= (long long)g5) && ((unsigned long long)w3)))) ? (unsigned int)((unsigned int)(long long)g3 | (unsigned int)(long long)0ull) : (unsigned long long)((unsigned long long)(long long)3ull << ((unsigned)(unsigned int)1ull & 63))) / ((unsigned int)(unsigned char)((unsigned int)(int)((unsigned int)(unsigned int)2ull - (unsigned int)(unsigned long long)32767ull) & (unsigned int)(signed char)(((short)l0 == (unsigned char)w3))) | 1u));
            w3--;
        } }
        break;
    case 3:
        g5 = (signed char)((((unsigned char)ga1[2] >= (unsigned long long)g3) && (!((unsigned int)255ull > (int)g5))) ? (unsigned long long)((unsigned long long)(unsigned short)l0 * (unsigned long long)(short)g6) : (short)((unsigned int)(unsigned long long)7ull >> ((unsigned)(unsigned int)128ull & 31)));
    default:
        ga1[1] = (unsigned int)((unsigned int)(int)((unsigned int)(signed char)(((unsigned char)ga1[1] > (short)255ull) ? (short)g10 : (long long)l0) + (unsigned int)(unsigned short)f1((long long)ga0[0], (signed char)256ull, (unsigned long long)1ull, (long long)1ull)) * (unsigned int)(int)f1((long long)g10, (signed char)((unsigned int)(short)7ull / ((unsigned int)(unsigned char)0ull | 1u)), (unsigned long long)((unsigned long long)(unsigned int)65535ull + (unsigned long long)(long long)g3), (long long)((unsigned long long)0 - (unsigned long long)(int)g8)));
    }
    { int w4 = 2; while (w4 > 0) {
        g8 = (int)((unsigned int)(unsigned int)((unsigned int)0 - (unsigned int)(unsigned char)(~(unsigned int)(unsigned int)p0)) | (unsigned int)(short)f1((long long)(((!((unsigned int)g0 <= (short)p1)) || ((unsigned long long)3ull < (unsigned char)g3)) ? (int)ga1[2] : (unsigned short)0ull), (signed char)(~(unsigned int)(long long)255ull), (unsigned long long)g10, (long long)f1((long long)p1, (signed char)w4, (unsigned long long)127ull, (long long)w4)));
        w4--;
    } }
    return (signed char)g1;
}

int main(void) {
    unsigned long long m0 = (unsigned long long)255ull;
    short m1 = (short)127ull;
    g4 = (unsigned long long)((unsigned long long)(unsigned short)m1 / ((unsigned long long)(unsigned long long)ga1[1] | 1u));
    g4 = (unsigned long long)((unsigned long long)(long long)(~(unsigned long long)(unsigned short)(~(unsigned int)(unsigned short)32767ull)) & (unsigned long long)(unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(signed char)g4 ^ (unsigned long long)(long long)ga0[4]) - (unsigned int)(unsigned long long)((unsigned long long)(short)3ull & (unsigned long long)(unsigned short)3ull)));
    m1 = (short)f0((unsigned short)((unsigned int)(unsigned char)1ull * (unsigned int)(int)256ull), (unsigned short)g1, (long long)((unsigned long long)(unsigned char)ga0[4] - (unsigned long long)(signed char)g8), (short)((unsigned int)(unsigned short)g7 << ((unsigned)(unsigned int)g9 & 31)));
    for (int i0 = 0; i0 < 6; i0++) {
        g6 = (short)((unsigned int)(signed char)(((short)((unsigned int)(unsigned char)256ull + (unsigned int)(unsigned short)ga0[0]))) & (unsigned int)(short)(((long long)((((((unsigned int)ga1[0] != (signed char)127ull) || ((short)ga0[3] <= (unsigned long long)0ull)) && (((unsigned long long)i0 < (short)100ull) && ((unsigned int)g8 != (unsigned long long)ga0[3]))) || ((short)ga0[2] <= (long long)ga0[3])))) ? (long long)((unsigned long long)(unsigned short)g0 >> ((unsigned)(unsigned int)15ull & 63)) : (short)((unsigned int)(unsigned long long)1ull << ((unsigned)(unsigned int)g10 & 31))));
        { int w1 = 4; while (w1 > 0) {
            if ((short)ga0[0] >= (long long)(((short)((unsigned int)(signed char)127ull >> ((unsigned)(unsigned int)w1 & 31)) != (unsigned char)3ull))) {
                g4 = (unsigned long long)g0;
                g2 = (unsigned int)(((int)127ull >= (long long)1ull) ? (short)ga1[2] : (int)1727407150ull);
                g4 = (unsigned long long)((unsigned long long)(short)f1((long long)(((unsigned int)i0 == (unsigned char)256ull) ? (unsigned int)7ull : (unsigned int)ga1[0]), (signed char)255ull, (unsigned long long)((unsigned long long)(short)g3 ^ (unsigned long long)(unsigned long long)g2), (long long)(((unsigned long long)7ull) ? (signed char)ga0[0] : (int)256ull)) + (unsigned long long)(int)2ull);
            }
            w1--;
        } }
    }
    g8 = (int)((unsigned int)(signed char)15ull % ((unsigned int)(unsigned long long)100ull | 1u));
    if (((signed char)((((unsigned int)g8 == (unsigned short)181ull) && (((((unsigned long long)127ull) || ((short)ga1[1] >= (long long)128ull)) && (((unsigned long long)g8 > (long long)g0) && ((int)128ull < (unsigned long long)255ull))) && ((short)g5 != (signed char)g7)))) >= (unsigned int)(((unsigned int)g4 > (long long)m1) ? (long long)m0 : (unsigned int)ga0[0])) && (((short)g8 == (unsigned int)ga1[2]) && ((unsigned short)256ull))) {
        if (!((unsigned long long)((unsigned long long)(short)g1 << ((unsigned)(unsigned int)g2 & 63)) == (long long)g10)) {
            for (int i2 = 0; i2 < 3; i2++) {
                g1 = (long long)((unsigned long long)(long long)((unsigned long long)(unsigned char)((unsigned int)(int)(((unsigned char)g3 == (signed char)g3) ? (int)255ull : (unsigned short)ga1[0]) >> ((unsigned)(unsigned int)((unsigned int)(int)ga1[1] - (unsigned int)(unsigned char)3ull) & 31)) * (unsigned long long)(long long)((unsigned long long)(long long)((((short)g4 > (unsigned int)256ull) || (!((((int)127ull < (unsigned int)4283460252ull) && (((long long)32767ull >= (short)i2) || (((long long)g8 == (unsigned short)128ull) && (((((short)2ull >= (unsigned char)0ull) || (((((int)ga1[0] == (signed char)255ull) || ((!((unsigned long long)ga1[2] <= (unsigned int)m0)) || ((((long long)3ull == (int)ga0[4]) && ((unsigned long long)0ull > (unsigned short)ga0[2])) && (((((long long)g11 != (short)3ull) && (((unsigned long long)65535ull < (unsigned char)ga0[3]) && (((signed char)7ull) && (((((unsigned char)118ull == (int)7ull) && (((signed char)256ull > (short)1ull) && ((unsigned short)g10))) && ((unsigned char)65535ull >= (int)m1)) && ((unsigned char)g3 >= (long long)g4))))) && ((long long)ga1[2] >= (unsigned char)ga1[0])) && (((((int)g2 == (signed char)256ull) || ((signed char)ga0[3])) || (((long long)128ull < (int)2ull) || ((long long)i2 < (long long)1ull))) && ((((long long)g1 < (signed char)g6) && ((unsigned short)2ull < (signed char)g10)) && (((unsigned long long)g2 != (unsigned short)g10) && ((unsigned long long)879377948444515129ull == (unsigned short)256ull)))))))) && ((((((unsigned long long)g8 >= (signed char)214ull) && (((int)0ull) || ((unsigned int)ga0[4] <= (short)1ull))) || ((short)g3 <= (int)g4)) || (((signed char)0ull >= (unsigned int)3ull) && ((unsigned char)2ull <= (unsigned int)g9))) && ((long long)3ull < (unsigned long long)g2))) && (((unsigned long long)g6 == (int)g0) && ((signed char)g9 == (int)ga1[2])))) && (((unsigned int)g11 < (short)g2) && ((int)2ull <= (long long)ga1[1]))) && ((short)2ull != (long long)g2))))) || ((((long long)g0 > (long long)m1) && ((unsigned long long)255ull >= (unsigned char)0ull)) || ((signed char)g9 > (short)g8)))))) - (unsigned long long)(unsigned long long)(((unsigned short)g4 < (short)15ull) ? (unsigned short)ga0[0] : (unsigned short)g0))) * (unsigned long long)(short)((unsigned int)(int)((unsigned int)(short)((unsigned int)0 - (unsigned int)(long long)ga0[0]) * (unsigned int)(unsigned int)((unsigned int)(long long)3ull / ((unsigned int)(unsigned long long)ga1[0] | 1u))) + (unsigned int)(long long)((unsigned long long)(unsigned char)(((short)g8 > (int)2ull)) ^ (unsigned long long)(signed char)((unsigned int)(int)15ull - (unsigned int)(unsigned long long)100ull))));
                m0 = (unsigned long long)((((unsigned short)((unsigned int)(long long)((unsigned long long)(unsigned char)ga1[0] & (unsigned long long)(unsigned short)100ull) / ((unsigned int)(signed char)255ull | 1u)) >= (unsigned char)((unsigned int)(signed char)g7 >> ((unsigned)(unsigned int)((unsigned int)(short)32767ull % ((unsigned int)(int)g9 | 1u)) & 31))) && (((unsigned char)((unsigned int)(signed char)32767ull % ((unsigned int)(unsigned char)g2 | 1u)) == (signed char)((unsigned int)0 - (unsigned int)(unsigned char)ga1[2])) || (((!(((unsigned int)g9 >= (short)g11) || ((int)113ull == (unsigned short)ga0[0]))) && ((((int)ga0[1] >= (signed char)ga1[1]) && ((unsigned long long)ga1[0] > (signed char)128ull)) || ((unsigned char)ga0[2] < (signed char)65535ull))) || ((int)1785235557ull != (unsigned short)m0)))) ? (unsigned short)((unsigned int)(unsigned long long)f1((long long)((unsigned long long)(unsigned char)g0 & (unsigned long long)(signed char)ga0[1]), (signed char)f0((unsigned short)g2, (unsigned short)7ull, (long long)g10, (short)1ull), (unsigned long long)g5, (long long)((unsigned long long)(unsigned int)g11 / ((unsigned long long)(short)65535ull | 1u))) * (unsigned int)(unsigned char)7ull) : (long long)((unsigned long long)(long long)(((long long)((unsigned long long)(signed char)ga1[1] ^ (unsigned long long)(signed char)3ull) > (unsigned short)((unsigned int)(int)g6 | (unsigned int)(unsigned short)15ull)) ? (short)((unsigned int)(short)g4 / ((unsigned int)(unsigned long long)127ull | 1u)) : (unsigned long long)((((unsigned int)65535ull < (int)3ull) && ((short)188ull)) ? (unsigned char)ga1[2] : (int)g9)) + (unsigned long long)(unsigned char)g2));
            }
            g2 = (unsigned int)g7;
        } else {
            g8 = (int)((unsigned int)(unsigned char)ga0[2] ^ (unsigned int)(unsigned char)g11);
            switch ((int)((int)((((unsigned long long)15ull > (int)32767ull) && (((int)g6 != (signed char)g8) || ((signed char)g6 > (unsigned short)g6))) ? (unsigned int)((unsigned int)(unsigned short)g4 % ((unsigned int)(unsigned long long)g1 | 1u)) : (long long)f1((long long)100ull, (signed char)g5, (unsigned long long)ga1[0], (long long)ga0[4]))) & 7) {
            case 0:
                g11 = (unsigned char)(((((unsigned int)m1 > (unsigned char)g2) && ((signed char)m1 <= (long long)2ull)) || ((signed char)110ull <= (unsigned long long)g2)) ? (unsigned short)g8 : (unsigned int)g8);
                break;
            case 3:
                g7 = (unsigned int)g11;
                break;
            case 4:
                g1 = (long long)((unsigned long long)(int)((unsigned int)(long long)((unsigned long long)(signed char)65535ull ^ (unsigned long long)(unsigned int)188ull) % ((unsigned int)(int)g5 | 1u)) * (unsigned long long)(short)((unsigned int)(long long)((unsigned long long)(unsigned char)ga1[0] >> ((unsigned)(unsigned int)g7 & 63)) + (unsigned int)(signed char)((unsigned int)(long long)g3 << ((unsigned)(unsigned int)0ull & 31))));
                break;
            default:
                g8 = (int)((unsigned int)(short)((unsigned int)(unsigned char)ga1[1] & (unsigned int)(unsigned short)((unsigned int)(unsigned int)((unsigned int)(short)g2 & (unsigned int)(short)g4) / ((unsigned int)(unsigned char)g7 | 1u))) ^ (unsigned int)(unsigned long long)((unsigned long long)(short)(((unsigned long long)g2 != (short)1ull)) ^ (unsigned long long)(long long)((unsigned long long)(long long)((((((unsigned long long)g8 <= (long long)g7) && ((int)127ull >= (unsigned int)g2)) && ((long long)7ull == (short)g9)) && ((unsigned int)2ull >= (short)65535ull))) + (unsigned long long)(unsigned int)((((short)1ull != (unsigned long long)127ull) || ((unsigned int)g10 != (unsigned long long)ga1[2])) ? (unsigned int)3ull : (unsigned char)g10))));
            }
        }
        g0 = (unsigned short)m1;
        if ((int)((unsigned int)(signed char)((unsigned int)(long long)g9 ^ (unsigned int)(signed char)g8) & (unsigned int)(unsigned long long)ga1[2]) <= (unsigned long long)((unsigned long long)(short)((((int)ga1[0] != (short)ga0[0]) || ((unsigned char)g8 == (long long)g6)) ? (short)3ull : (unsigned short)g4) / ((unsigned long long)(unsigned int)32767ull | 1u))) {
            if ((short)((((unsigned int)15ull == (signed char)123ull) || (((unsigned long long)g1) || (((signed char)g8 != (unsigned long long)g4) && ((long long)2ull))))) < (unsigned long long)g8) {
                ga1[0] = (unsigned int)((unsigned int)(signed char)((unsigned int)(int)((unsigned int)0 - (unsigned int)(int)ga1[1]) * (unsigned int)(signed char)f0((unsigned short)ga1[2], (unsigned short)15ull, (long long)14380332481647648831ull, (short)ga0[4])) + (unsigned int)(short)f2((unsigned long long)ga0[1], (unsigned long long)ga1[1]));
                g8 = (int)((unsigned int)(short)((!((long long)((unsigned long long)(short)m0 / ((unsigned long long)(unsigned long long)g8 | 1u)) < (unsigned int)((unsigned int)0 - (unsigned int)(short)g1))) ? (unsigned char)((((unsigned char)127ull >= (short)m1) && ((int)g9 < (signed char)0ull)) ? (short)((unsigned int)(long long)m1 | (unsigned int)(unsigned short)ga1[1]) : (unsigned long long)((unsigned long long)(unsigned short)32767ull - (unsigned long long)(unsigned short)g1)) : (long long)((unsigned long long)(unsigned long long)(~(unsigned long long)(long long)ga0[3]) & (unsigned long long)(unsigned long long)((unsigned long long)(short)g7 | (unsigned long long)(unsigned short)128ull))) * (unsigned int)(unsigned int)f0((unsigned short)((unsigned int)(short)((unsigned int)(short)100ull | (unsigned int)(long long)g6) % ((unsigned int)(unsigned int)((unsigned int)(unsigned long long)g2 * (unsigned int)(long long)g3) | 1u)), (unsigned short)((unsigned int)(unsigned char)((unsigned int)(unsigned short)m1 << ((unsigned)(unsigned int)127ull & 31)) & (unsigned int)(signed char)(~(unsigned int)(long long)ga0[2])), (long long)((unsigned long long)(unsigned char)m0 << ((unsigned)(unsigned int)f0((unsigned short)ga1[2], (unsigned short)g1, (long long)128ull, (short)1ull) & 63)), (short)f1((long long)(((int)2ull > (short)ga1[2])), (signed char)((unsigned int)(unsigned int)g3 - (unsigned int)(unsigned short)g11), (unsigned long long)(((unsigned char)32767ull <= (long long)g7) ? (int)3ull : (unsigned short)ga1[2]), (long long)(((long long)65535ull == (unsigned short)g8)))));
                g10 = (unsigned char)g0;
            }
        }
    } else {
        for (int i3 = 0; i3 < 1; i3++) {
            m0 = (unsigned long long)((unsigned long long)(unsigned int)((unsigned int)0 - (unsigned int)(unsigned short)f2((unsigned long long)(((unsigned int)7ull < (short)ga1[0])), (unsigned long long)1ull)) | (unsigned long long)(unsigned short)f0((unsigned short)128ull, (unsigned short)((unsigned int)(signed char)((unsigned int)(int)m1 / ((unsigned int)(unsigned long long)m0 | 1u)) - (unsigned int)(unsigned char)((unsigned int)(unsigned char)ga0[2] + (unsigned int)(unsigned short)128ull)), (long long)((unsigned long long)(unsigned long long)f2((unsigned long long)ga1[0], (unsigned long long)m0) - (unsigned long long)(unsigned char)g3), (short)(~(unsigned int)(long long)m0)));
        }
    }
    ga1[1] = (unsigned int)((unsigned int)0 - (unsigned int)(signed char)((unsigned int)(short)((unsigned int)(signed char)ga0[4] + (unsigned int)(long long)g5) / ((unsigned int)(short)((unsigned int)(unsigned int)ga1[0] << ((unsigned)(unsigned int)g6 & 31)) | 1u)));
    if ((int)((unsigned int)(unsigned short)((unsigned int)(long long)7ull >> ((unsigned)(unsigned int)m1 & 31)) - (unsigned int)(unsigned int)((unsigned int)(unsigned long long)ga0[2] - (unsigned int)(short)255ull)) != (unsigned long long)((unsigned long long)(short)(((!(((unsigned int)g4 > (short)128ull) && ((long long)g9))) || (((unsigned long long)65535ull < (unsigned char)2ull) && (((signed char)100ull == (unsigned char)g1) || ((unsigned short)g11 == (unsigned char)ga0[3]))))) ^ (unsigned long long)(unsigned int)((unsigned int)(int)65535ull >> ((unsigned)(unsigned int)127ull & 31)))) {
        switch ((int)((int)(~(unsigned int)(unsigned short)((unsigned int)(int)m1 ^ (unsigned int)(long long)15ull))) & 7) {
        case 0:
            { int w4 = 1; while (w4 > 0) {
                g4 = (unsigned long long)((unsigned long long)(int)32767ull % ((unsigned long long)(short)g10 | 1u));
                w4--;
            } }
        case 3:
            m0 = (unsigned long long)((unsigned long long)(unsigned long long)2ull - (unsigned long long)(long long)((((unsigned long long)g11 <= (signed char)ga1[1]) || ((unsigned char)85ull >= (signed char)161ull))));
            break;
        case 4:
            switch ((int)((int)f1((long long)((unsigned long long)(short)128ull | (unsigned long long)(unsigned short)ga0[2]), (signed char)((unsigned int)(unsigned short)2ull >> ((unsigned)(unsigned int)g1 & 31)), (unsigned long long)((unsigned long long)(unsigned short)m0 >> ((unsigned)(unsigned int)ga1[1] & 63)), (long long)g5)) & 7) {
            case 0:
                g6 = (short)((unsigned int)(short)201ull ^ (unsigned int)(unsigned char)128ull);
            case 7:
                g4 = (unsigned long long)((unsigned long long)(long long)g10 | (unsigned long long)(unsigned int)g11);
                break;
            default:
                g2 = (unsigned int)f1((long long)g1, (signed char)7ull, (unsigned long long)g10, (long long)7ull);
            }
            break;
        case 6:
            if ((signed char)((unsigned int)(unsigned long long)f0((unsigned short)g4, (unsigned short)g10, (long long)g5, (short)1ull) * (unsigned int)(unsigned char)((unsigned int)(int)g5 >> ((unsigned)(unsigned int)256ull & 31))) > (unsigned char)((unsigned int)(int)1ull >> ((unsigned)(unsigned int)((((long long)g0) || ((long long)m0 <= (unsigned char)g3)) ? (short)g1 : (unsigned char)ga1[1]) & 31))) {
                g10 = (unsigned char)(((unsigned int)((unsigned int)(short)g6 & (unsigned int)(unsigned int)g8) < (short)((unsigned int)0 - (unsigned int)(short)ga1[0])) ? (long long)((unsigned long long)0 - (unsigned long long)(short)127ull) : (long long)((unsigned long long)(unsigned char)15ull * (unsigned long long)(signed char)g8));
                g2 = (unsigned int)ga1[1];
            } else {
                g5 = (signed char)((unsigned int)(unsigned char)((unsigned int)(short)1ull / ((unsigned int)(unsigned int)m0 | 1u)) & (unsigned int)(int)f1((long long)g2, (signed char)g6, (unsigned long long)32767ull, (long long)197ull));
                g5 = (signed char)g8;
            }
            break;
        default:
            g7 = (unsigned int)((unsigned int)(int)((unsigned int)(unsigned short)(((long long)g1) ? (long long)ga1[0] : (short)g10) & (unsigned int)(unsigned long long)((unsigned long long)(signed char)1ull * (unsigned long long)(unsigned char)127ull)) & (unsigned int)(signed char)15ull);
        }
        if ((((unsigned short)3ull <= (int)ga0[2]) || ((((int)2ull <= (unsigned char)235ull) || ((unsigned int)65535ull > (int)ga0[2])) || ((unsigned int)96ull))) || ((short)((unsigned int)(short)ga1[2] - (unsigned int)(long long)2ull) <= (unsigned char)ga1[2])) {
            ga0[3] = (signed char)((unsigned int)(unsigned int)f0((unsigned short)g0, (unsigned short)((unsigned int)(short)g9 - (unsigned int)(int)g1), (long long)((unsigned long long)(unsigned char)g8 + (unsigned long long)(unsigned short)ga0[3]), (short)f1((long long)ga0[1], (signed char)3ull, (unsigned long long)0ull, (long long)100ull)) & (unsigned int)(signed char)((unsigned int)(long long)(((unsigned long long)ga0[0] <= (unsigned short)m0) ? (unsigned long long)g10 : (unsigned short)g4) * (unsigned int)(unsigned short)((unsigned int)(unsigned long long)ga1[0] + (unsigned int)(unsigned short)4720ull)));
        } else {
            for (int i5 = 0; i5 < 5; i5++) {
                m0 = (unsigned long long)((unsigned long long)0 - (unsigned long long)(unsigned char)((unsigned int)(long long)ga0[3] / ((unsigned int)(short)(((unsigned char)((unsigned int)(unsigned int)g0 & (unsigned int)(unsigned long long)127ull) >= (long long)f1((long long)m0, (signed char)ga1[1], (unsigned long long)ga1[1], (long long)3ull)) ? (long long)((unsigned long long)(short)7ull / ((unsigned long long)(unsigned short)100ull | 1u)) : (unsigned long long)g0) | 1u)));
            }
            for (int i6 = 0; i6 < 4; i6++) {
                g6 = (short)((unsigned int)(int)((unsigned int)(int)((unsigned int)(unsigned int)196ull >> ((unsigned)(unsigned int)ga1[2] & 31)) + (unsigned int)(int)((unsigned int)(unsigned char)g6 - (unsigned int)(unsigned int)32767ull)) >> ((unsigned)(unsigned int)((unsigned int)(unsigned long long)((unsigned long long)(unsigned short)255ull * (unsigned long long)(signed char)g4) & (unsigned int)(signed char)f2((unsigned long long)i6, (unsigned long long)255ull)) & 31));
                g4 = (unsigned long long)((unsigned long long)(long long)((unsigned long long)(unsigned long long)255ull >> ((unsigned)(unsigned int)g7 & 63)) & (unsigned long long)(int)65535ull);
            }
        }
        for (int i7 = 0; i7 < 5; i7++) {
            g3 = (short)((unsigned int)(unsigned char)((unsigned int)(int)((unsigned int)(short)(~(unsigned int)(short)g3) | (unsigned int)(int)((unsigned int)(unsigned char)g1 - (unsigned int)(long long)ga0[0])) & (unsigned int)(signed char)((unsigned int)(unsigned long long)((unsigned long long)(long long)g4 ^ (unsigned long long)(unsigned char)ga0[4]) << ((unsigned)(unsigned int)(~(unsigned int)(unsigned short)127ull) & 31))) ^ (unsigned int)(int)((unsigned int)(short)g7 + (unsigned int)(unsigned short)(((short)((unsigned int)0 - (unsigned int)(short)g3) <= (short)((unsigned int)0 - (unsigned int)(short)32767ull)) ? (short)((unsigned int)(long long)3ull & (unsigned int)(signed char)g8) : (int)g1)));
            for (int i8 = 0; i8 < 3; i8++) {
                ga0[0] = (signed char)f1((long long)f1((long long)(~(unsigned long long)(unsigned char)ga0[3]), (signed char)65535ull, (unsigned long long)((unsigned long long)(unsigned int)ga1[1] >> ((unsigned)(unsigned int)100ull & 63)), (long long)((unsigned long long)(signed char)255ull & (unsigned long long)(unsigned int)ga0[1])), (signed char)g11, (unsigned long long)((unsigned long long)(unsigned char)((((short)ga0[2] > (unsigned long long)i7) && ((unsigned short)29695ull)) ? (short)g0 : (int)2ull) + (unsigned long long)(unsigned char)g5), (long long)((unsigned long long)(short)(((unsigned int)ga0[3] >= (int)ga0[1]) ? (unsigned short)i7 : (signed char)ga0[4]) ^ (unsigned long long)(unsigned char)((((((((unsigned char)256ull) && (((int)62ull != (int)g3) || ((signed char)g7 != (int)ga1[1]))) && ((long long)32767ull == (unsigned short)1ull)) && ((unsigned long long)g6 <= (unsigned long long)ga0[3])) && ((!((unsigned char)128ull)) && ((unsigned char)g2 != (unsigned char)216ull))) && (!((!((unsigned char)g10 >= (unsigned char)ga1[1])) || (!(((short)ga0[2] < (short)g6) && ((unsigned short)3ull != (unsigned int)g8)))))))));
                g0 = (unsigned short)((unsigned int)(unsigned char)((unsigned int)(long long)256ull * (unsigned int)(unsigned short)((((((((unsigned short)g3 > (unsigned int)ga1[0]) || ((unsigned int)g1 == (unsigned char)g9)) && ((((unsigned int)g3 != (int)m0) || ((unsigned long long)ga0[2] < (unsigned long long)255ull)) && (((long long)256ull != (int)m0) && ((unsigned char)32767ull > (signed char)15ull)))) && ((long long)15ull >= (int)g8)) || ((((signed char)255ull == (unsigned short)ga0[1]) && ((((int)g10 > (short)ga0[3]) || ((short)ga1[0] == (short)g11)) || (((unsigned long long)100ull == (short)g1) || ((!((!(((int)g5 < (int)g3) || (((signed char)m0) || (((short)15ull < (unsigned long long)ga1[0]) && ((long long)i7 >= (unsigned short)g11))))) || (((signed char)g9) || ((((unsigned long long)g10 < (unsigned char)ga0[4]) || (((unsigned short)1ull != (unsigned char)1ull) && (((signed char)3ull <= (int)844795681ull) || (!((!((unsigned long long)3ull == (long long)13761906810483573561ull)) && (((unsigned short)2ull != (int)g10) && ((((unsigned long long)65535ull) && (((unsigned int)ga1[0] >= (int)i8) || ((short)ga0[0] != (signed char)g6))) && (((int)g8 >= (unsigned char)g4) || ((unsigned char)234ull))))))))) && ((short)32767ull <= (long long)g3))))) && ((long long)ga1[0] != (signed char)g5))))) && (((short)2ull != (long long)65535ull) && ((((unsigned char)g3 < (long long)ga1[0]) || ((((((signed char)15ull > (int)127ull) && (!((unsigned char)94ull == (unsigned short)1ull))) || (((long long)128ull < (unsigned long long)127ull) || ((short)ga0[0] > (short)g5))) && (((unsigned char)ga1[1] != (signed char)1ull) || ((int)215ull))) && ((long long)g8 >= (long long)g10))) && ((short)g6 <= (unsigned short)g5))))) && (((long long)ga1[2]) && ((unsigned long long)256ull < (unsigned int)i7))))) / ((unsigned int)(int)((unsigned int)(short)((unsigned int)(unsigned int)((unsigned int)(unsigned short)g6 >> ((unsigned)(unsigned int)i7 & 31)) | (unsigned int)(unsigned short)((unsigned int)0 - (unsigned int)(int)i7)) << ((unsigned)(unsigned int)((unsigned int)0 - (unsigned int)(unsigned int)((((unsigned int)g1 >= (signed char)g8) && ((((unsigned long long)9ull < (short)g4) || ((((((int)3ull <= (signed char)g11) || ((int)ga0[3] == (long long)g3)) && ((long long)256ull != (short)65535ull)) || ((unsigned int)7ull)) || ((unsigned long long)256ull))) && (((((int)g1 != (long long)g1) || ((unsigned char)1ull >= (short)65535ull)) || ((int)128ull == (short)1ull)) && ((int)ga0[4] >= (signed char)ga1[2])))) ? (unsigned short)ga0[1] : (unsigned short)100ull)) & 31)) | 1u));
            }
            switch ((int)((int)((unsigned int)(short)((unsigned int)(unsigned char)256ull << ((unsigned)(unsigned int)g4 & 31)) / ((unsigned int)(unsigned int)((unsigned int)0 - (unsigned int)(int)g11) | 1u))) & 7) {
            case 4:
                g5 = (signed char)((unsigned int)(int)((unsigned int)(short)51645ull % ((unsigned int)(unsigned char)255ull | 1u)) / ((unsigned int)(unsigned int)(((unsigned long long)((unsigned long long)(int)g8 | (unsigned long long)(unsigned int)ga0[3]) < (unsigned int)255ull)) | 1u));
                break;
            case 6:
                g6 = (short)((unsigned int)0 - (unsigned int)(long long)((unsigned long long)(unsigned int)(~(unsigned int)(unsigned long long)g7) ^ (unsigned long long)(unsigned short)((unsigned int)(unsigned long long)1ull + (unsigned int)(short)127ull)));
                break;
            default:
                g10 = (unsigned char)(((signed char)((unsigned int)0 - (unsigned int)(unsigned short)((unsigned int)(unsigned char)g11 + (unsigned int)(unsigned int)4031640358ull)) >= (unsigned short)m1));
            }
        }
    }
    g10 = (unsigned char)((((long long)((unsigned long long)(long long)(((unsigned int)180ull < (unsigned char)1ull) ? (long long)m0 : (int)g9) << ((unsigned)(unsigned int)((unsigned int)(short)ga1[0] % ((unsigned int)(long long)ga1[0] | 1u)) & 63))) && ((unsigned short)((unsigned int)(long long)2ull * (unsigned int)(unsigned short)((unsigned int)(short)g1 | (unsigned int)(unsigned int)127ull)))) ? (unsigned long long)g7 : (signed char)((unsigned int)(int)((!((unsigned int)32767ull == (unsigned long long)ga1[1])) ? (unsigned char)((unsigned int)(int)g3 + (unsigned int)(unsigned long long)172ull) : (short)((unsigned int)(short)2ull % ((unsigned int)(short)128ull | 1u))) / ((unsigned int)(long long)(((unsigned long long)((unsigned long long)(signed char)g5 / ((unsigned long long)(int)256ull | 1u)) <= (short)f2((unsigned long long)ga0[3], (unsigned long long)g7))) | 1u)));
    m1 = (short)(~(unsigned int)(long long)1ull);
    g8 = (int)(((unsigned short)((unsigned int)(signed char)((unsigned int)(long long)ga1[0] % ((unsigned int)(unsigned short)g6 | 1u)) * (unsigned int)(unsigned long long)((unsigned long long)(unsigned short)255ull & (unsigned long long)(int)255ull)) == (long long)(((short)f1((long long)g1, (signed char)g6, (unsigned long long)g8, (long long)g1) >= (int)2ull) ? (unsigned char)f2((unsigned long long)g7, (unsigned long long)7ull) : (int)(((signed char)ga0[2] < (signed char)g6) ? (unsigned char)7ull : (unsigned short)127ull))) ? (signed char)f1((long long)g1, (signed char)((unsigned int)(signed char)g10 - (unsigned int)(int)256ull), (unsigned long long)g10, (long long)f2((unsigned long long)ga0[3], (unsigned long long)ga1[0])) : (unsigned long long)f2((unsigned long long)f1((long long)100ull, (signed char)ga1[1], (unsigned long long)g7, (long long)g0), (unsigned long long)((unsigned long long)(short)g5 * (unsigned long long)(int)255ull)));
    if ((short)(((signed char)((unsigned int)(unsigned int)g1 >> ((unsigned)(unsigned int)63ull & 31)) >= (signed char)(~(unsigned int)(short)0ull)) ? (signed char)((unsigned int)(unsigned char)0ull ^ (unsigned int)(unsigned short)128ull) : (unsigned long long)((unsigned long long)(unsigned int)g4 / ((unsigned long long)(int)ga0[4] | 1u))) > (int)((unsigned int)(unsigned int)((unsigned int)(signed char)g4 & (unsigned int)(int)ga0[1]) % ((unsigned int)(int)(((int)256ull <= (long long)ga0[3]) ? (unsigned short)ga0[2] : (unsigned char)15ull) | 1u))) {
        g10 = (unsigned char)(((short)g0 == (unsigned long long)((unsigned long long)(signed char)ga0[4] & (unsigned long long)(signed char)((unsigned int)(signed char)g1 ^ (unsigned int)(unsigned short)31107ull))));
    } else {
        g2 = (unsigned int)g10;
    }
    g10 = (unsigned char)f2((unsigned long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned int)117ull - (unsigned int)(short)g10) ^ (unsigned long long)(unsigned int)f0((unsigned short)((unsigned int)(unsigned int)256ull * (unsigned int)(unsigned char)2ull), (unsigned short)((unsigned int)(short)g1 >> ((unsigned)(unsigned int)15ull & 31)), (long long)((unsigned long long)0 - (unsigned long long)(signed char)2ull), (short)g3)), (unsigned long long)((unsigned long long)(int)((unsigned int)(unsigned short)f2((unsigned long long)m1, (unsigned long long)127ull) >> ((unsigned)(unsigned int)(~(unsigned int)(unsigned long long)32767ull) & 31)) | (unsigned long long)(unsigned char)(~(unsigned int)(unsigned char)((!((unsigned char)2ull))))));
    runtime.printf("g0=%llx\n", (unsigned long long)g0);
    runtime.printf("g1=%llx\n", (unsigned long long)g1);
    runtime.printf("g2=%llx\n", (unsigned long long)g2);
    runtime.printf("g3=%llx\n", (unsigned long long)g3);
    runtime.printf("g4=%llx\n", (unsigned long long)g4);
    runtime.printf("g5=%llx\n", (unsigned long long)g5);
    runtime.printf("g6=%llx\n", (unsigned long long)g6);
    runtime.printf("g7=%llx\n", (unsigned long long)g7);
    runtime.printf("g8=%llx\n", (unsigned long long)g8);
    runtime.printf("g9=%llx\n", (unsigned long long)g9);
    runtime.printf("g10=%llx\n", (unsigned long long)g10);
    runtime.printf("g11=%llx\n", (unsigned long long)g11);
    runtime.printf("m0=%llx\n", (unsigned long long)m0);
    runtime.printf("m1=%llx\n", (unsigned long long)m1);
    runtime.printf("ga0_0=%llx\n", (unsigned long long)ga0[0]);
    runtime.printf("ga0_1=%llx\n", (unsigned long long)ga0[1]);
    runtime.printf("ga0_2=%llx\n", (unsigned long long)ga0[2]);
    runtime.printf("ga0_3=%llx\n", (unsigned long long)ga0[3]);
    runtime.printf("ga0_4=%llx\n", (unsigned long long)ga0[4]);
    runtime.printf("ga1_0=%llx\n", (unsigned long long)ga1[0]);
    runtime.printf("ga1_1=%llx\n", (unsigned long long)ga1[1]);
    runtime.printf("ga1_2=%llx\n", (unsigned long long)ga1[2]);
    return 0;
}

