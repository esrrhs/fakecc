package main;
import runtime;
long long g0 = (long long)1ull;
signed char g1 = (signed char)1ull;
unsigned short g2 = (unsigned short)233ull;
unsigned short g3 = (unsigned short)241ull;
long long g4 = (long long)14ull;
unsigned char g5 = (unsigned char)2ull;
int g6 = (int)32767ull;
unsigned short g7 = (unsigned short)2ull;
unsigned int g8 = (unsigned int)128ull;
unsigned int g9 = (unsigned int)128ull;
unsigned short ga0[3] = {(unsigned short)256ull, (unsigned short)3ull, (unsigned short)256ull};
long long ga1[8] = {(long long)255ull, (long long)127ull, (long long)7ull, (long long)256ull, (long long)32767ull, (long long)3ull, (long long)127ull, (long long)0ull};
static unsigned char f0(short p0) {
    unsigned char l0 = (unsigned char)((unsigned int)(unsigned int)((unsigned int)(signed char)g2 / ((unsigned int)(unsigned long long)ga0[0] | 1u)) % ((unsigned int)(long long)g5 | 1u));
    switch ((int)((int)((unsigned int)(unsigned short)18681ull | (unsigned int)(int)((unsigned int)(short)ga0[1] * (unsigned int)(unsigned short)g5))) & 7) {
    case 2:
        l0 = (unsigned char)((unsigned int)(int)3ull % ((unsigned int)(int)15ull | 1u));
        break;
    case 3:
        g1 = (signed char)((unsigned int)(unsigned int)15ull - (unsigned int)(long long)g9);
    case 6:
        g1 = (signed char)(~(unsigned int)(int)728286268ull);
        break;
    default:
        g4 = (long long)(~(unsigned long long)(long long)((unsigned long long)(unsigned long long)((((unsigned int)ga0[1] <= (int)0ull) && ((unsigned char)255ull <= (unsigned short)g7))) ^ (unsigned long long)(long long)((unsigned long long)(long long)((unsigned long long)(unsigned long long)g5 % ((unsigned long long)(long long)ga0[0] | 1u)) - (unsigned long long)(signed char)((unsigned int)(unsigned long long)ga1[5] * (unsigned int)(int)ga0[1]))));
    }
    ga1[1] = (long long)g7;
    ga1[1] = (long long)((unsigned long long)(unsigned int)(~(unsigned int)(unsigned char)((unsigned int)(unsigned char)ga0[0] ^ (unsigned int)(unsigned char)g8)) | (unsigned long long)(unsigned long long)((unsigned long long)(int)g2 - (unsigned long long)(int)g2));
    return (unsigned char)((unsigned int)(int)((unsigned int)(unsigned int)15ull * (unsigned int)(unsigned short)ga0[2]) ^ (unsigned int)(unsigned long long)ga1[3]);
}

static unsigned long long f1(unsigned long long p0, unsigned long long p1, int p2) {
    long long l0 = (long long)g2;
    int l1 = (int)((unsigned int)(long long)100ull << ((unsigned)(unsigned int)f0((short)g7) & 31));
    short l2 = (short)((unsigned int)(signed char)(~(unsigned int)(int)ga0[0]) | (unsigned int)(unsigned short)((unsigned int)(long long)g5 >> ((unsigned)(unsigned int)65535ull & 31)));
    p0 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(long long)255ull + (unsigned long long)(short)g7) / ((unsigned long long)(unsigned long long)(~(unsigned long long)(unsigned int)15ull) | 1u));
    g5 = (unsigned char)g6;
    for (int i0 = 0; i0 < 2; i0++) {
        g1 = (signed char)((unsigned int)(unsigned long long)g3 ^ (unsigned int)(unsigned int)((unsigned int)(signed char)((((signed char)ga0[2] >= (unsigned int)g0) || (((unsigned int)ga0[1] > (signed char)128ull) || (!((((((signed char)1ull < (int)l0) && (!((unsigned short)p2 < (unsigned int)ga1[3]))) || ((long long)5ull > (long long)ga1[6])) && (!((short)256ull == (short)g3))) || ((unsigned char)224ull != (unsigned char)ga1[1]))))) ? (long long)(~(unsigned long long)(unsigned int)100ull) : (unsigned char)((unsigned int)(unsigned long long)g0 ^ (unsigned int)(unsigned long long)p0)) + (unsigned int)(unsigned int)((unsigned int)(long long)ga0[0] * (unsigned int)(short)((unsigned int)(unsigned long long)ga0[0] ^ (unsigned int)(short)128ull))));
        l1 = (int)((unsigned int)(short)g1 + (unsigned int)(signed char)g5);
        g6 = (int)((unsigned int)(unsigned int)l2 * (unsigned int)(unsigned char)((unsigned int)(unsigned long long)ga1[3] | (unsigned int)(unsigned char)g6));
    }
    { int w1 = 4; while (w1 > 0) {
        g6 = (int)f0((short)(((short)((unsigned int)0 - (unsigned int)(long long)100ull) > (short)((unsigned int)(unsigned short)g7 / ((unsigned int)(unsigned int)g4 | 1u))) ? (long long)g4 : (signed char)(((int)ga0[1] > (unsigned char)ga1[2]) ? (unsigned short)32767ull : (unsigned int)0ull)));
        w1--;
    } }
    for (int i2 = 0; i2 < 3; i2++) {
        g8 = (unsigned int)((unsigned int)(unsigned short)15ull / ((unsigned int)(long long)((unsigned long long)(long long)3ull ^ (unsigned long long)(short)128ull) | 1u));
        switch ((int)((int)(((short)(((signed char)15ull < (unsigned char)g8) ? (unsigned char)32767ull : (unsigned short)g8) == (unsigned char)((unsigned int)(int)127ull | (unsigned int)(int)7ull)))) & 7) {
        case 0:
            g1 = (signed char)((unsigned int)(unsigned long long)(((unsigned short)g7 != (short)g8) ? (signed char)128ull : (unsigned char)ga0[2]) % ((unsigned int)(int)((unsigned int)(unsigned int)ga0[2] | (unsigned int)(int)p2) | 1u));
            break;
        case 2:
            p2 = (int)((unsigned int)(unsigned long long)((unsigned long long)0 - (unsigned long long)(unsigned char)((unsigned int)(long long)f0((short)3ull) | (unsigned int)(unsigned long long)((unsigned long long)(unsigned long long)ga1[6] * (unsigned long long)(unsigned long long)i2))) + (unsigned int)(unsigned char)p2);
            break;
        default:
            g6 = (int)((unsigned int)(long long)((unsigned long long)(short)((((unsigned char)100ull <= (short)ga1[4]) || (!((unsigned long long)15ull <= (unsigned long long)g5))) ? (unsigned long long)ga0[2] : (int)15ull) - (unsigned long long)(unsigned short)p0) >> ((unsigned)(unsigned int)((unsigned int)(signed char)((unsigned int)(unsigned long long)p2 + (unsigned int)(unsigned short)128ull) >> ((unsigned)(unsigned int)f0((short)2ull) & 31)) & 31));
        }
    }
    return (unsigned long long)((unsigned long long)0 - (unsigned long long)(unsigned char)(((int)2052859394ull >= (unsigned short)ga0[1]) ? (unsigned short)p2 : (unsigned long long)128ull));
}

static signed char f2(short p0, unsigned long long p1, long long p2) {
    unsigned short l0 = (unsigned short)f1((unsigned long long)g8, (unsigned long long)((unsigned long long)(unsigned long long)g5 / ((unsigned long long)(short)ga1[4] | 1u)), (int)((unsigned int)(short)g1 ^ (unsigned int)(signed char)g6));
    unsigned long long l1 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(unsigned char)65535ull / ((unsigned long long)(unsigned short)15ull | 1u)) - (unsigned long long)(int)447115276ull);
    short l2 = (short)(((long long)g1 != (unsigned char)g8) ? (short)f0((short)ga0[0]) : (long long)((((unsigned char)0ull) && ((unsigned int)ga1[1] > (unsigned long long)0ull)) ? (short)ga0[0] : (long long)127ull));
    g6 = (int)((unsigned int)(short)101ull | (unsigned int)(unsigned char)l1);
    g9 = (unsigned int)((unsigned int)(int)((unsigned int)(unsigned long long)((unsigned long long)(unsigned long long)7ull << ((unsigned)(unsigned int)g9 & 63)) ^ (unsigned int)(long long)(~(unsigned long long)(signed char)g2)) - (unsigned int)(long long)(((signed char)((unsigned int)(unsigned int)15ull + (unsigned int)(long long)0ull)) ? (int)(((long long)ga1[4] == (unsigned char)ga1[7]) ? (short)ga0[1] : (unsigned int)65535ull) : (long long)g4));
    g9 = (unsigned int)((unsigned int)(short)((unsigned int)(unsigned long long)((unsigned long long)(unsigned int)7ull - (unsigned long long)(long long)ga0[2]) << ((unsigned)(unsigned int)g5 & 31)) / ((unsigned int)(unsigned char)((unsigned int)(unsigned short)((unsigned int)(unsigned int)ga0[0] / ((unsigned int)(short)l1 | 1u)) << ((unsigned)(unsigned int)((unsigned int)(short)ga0[2] << ((unsigned)(unsigned int)ga1[6] & 31)) & 31)) | 1u));
    return (signed char)((unsigned int)(signed char)((unsigned int)(int)l1 >> ((unsigned)(unsigned int)l2 & 31)) % ((unsigned int)(unsigned int)f1((unsigned long long)32767ull, (unsigned long long)l2, (int)ga0[2]) | 1u));
}

static unsigned long long f3(int p0, unsigned char p1, int p2, signed char p3) {
    short l0 = (short)((unsigned int)(signed char)((unsigned int)(long long)1ull >> ((unsigned)(unsigned int)ga0[2] & 31)) - (unsigned int)(unsigned short)((unsigned int)(unsigned char)15ull / ((unsigned int)(unsigned int)256ull | 1u)));
    signed char l1 = (signed char)((unsigned int)(short)(((((short)3ull != (unsigned int)1ull) && (((int)g2 > (unsigned long long)p3) && ((unsigned int)g4 > (short)ga0[1]))) || ((long long)g0 != (int)15ull)) ? (unsigned char)ga0[1] : (unsigned short)g0) * (unsigned int)(short)g4);
    g5 = (unsigned char)((unsigned int)(unsigned char)((((unsigned int)g1 < (long long)p0) && ((signed char)g9 > (unsigned char)256ull)) ? (signed char)135ull : (short)p3) << ((unsigned)(unsigned int)(((unsigned char)l1 < (int)127ull)) & 31));
    g1 = (signed char)f2((short)((unsigned int)0 - (unsigned int)(short)((unsigned int)(long long)255ull % ((unsigned int)(unsigned char)127ull | 1u))), (unsigned long long)((unsigned long long)0 - (unsigned long long)(short)((unsigned int)(unsigned short)g5 >> ((unsigned)(unsigned int)15ull & 31))), (long long)((unsigned long long)(short)((unsigned int)(unsigned short)255ull << ((unsigned)(unsigned int)p0 & 31)) >> ((unsigned)(unsigned int)((unsigned int)(unsigned int)l1 - (unsigned int)(long long)g4) & 63)));
    return (unsigned long long)((unsigned long long)(unsigned int)f0((short)p0) / ((unsigned long long)(short)(~(unsigned int)(int)p1) | 1u));
}

int main(void) {
    signed char m0 = (signed char)45ull;
    long long m1 = (long long)2ull;
    unsigned short m2 = (unsigned short)256ull;
    signed char m3 = (signed char)255ull;
    switch ((int)((int)((unsigned int)(long long)((unsigned long long)(int)ga1[6] >> ((unsigned)(unsigned int)g9 & 63)) + (unsigned int)(unsigned char)(~(unsigned int)(unsigned short)32767ull))) & 7) {
    case 0:
        g8 = (unsigned int)((unsigned int)(unsigned long long)((unsigned long long)(short)((unsigned int)(unsigned short)6110ull / ((unsigned int)(unsigned int)ga1[0] | 1u)) % ((unsigned long long)(unsigned char)((unsigned int)(unsigned int)ga1[1] & (unsigned int)(signed char)g6) | 1u)) + (unsigned int)(signed char)((unsigned int)(unsigned int)(~(unsigned int)(unsigned char)212ull) / ((unsigned int)(long long)ga0[2] | 1u)));
    case 1:
        switch ((int)((int)((unsigned int)(int)((unsigned int)(short)59855ull >> ((unsigned)(unsigned int)ga1[1] & 31)) | (unsigned int)(unsigned int)f0((short)g2))) & 7) {
        case 0:
            ga1[3] = (long long)((unsigned long long)(long long)((((short)g9 < (unsigned int)255ull) && ((unsigned int)2ull < (int)256ull)) ? (long long)((((int)128ull < (unsigned int)128ull) && ((unsigned char)m2 == (unsigned int)15ull))) : (unsigned short)(((int)g8 > (unsigned long long)m1) ? (int)2352025588ull : (int)7ull)) << ((unsigned)(unsigned int)ga1[5] & 63));
        case 3:
            if ((unsigned short)((unsigned int)(int)((unsigned int)(short)m3 - (unsigned int)(int)ga1[4]) - (unsigned int)(unsigned int)((unsigned int)(unsigned long long)32ull | (unsigned int)(unsigned long long)g0)) >= (unsigned int)((unsigned int)(int)((unsigned int)(unsigned short)ga0[0] - (unsigned int)(unsigned int)32767ull) & (unsigned int)(int)((unsigned int)(int)32767ull ^ (unsigned int)(long long)ga1[3]))) {
                g5 = (unsigned char)((unsigned int)(long long)((unsigned long long)(signed char)g1 << ((unsigned)(unsigned int)g9 & 63)) % ((unsigned int)(short)g2 | 1u));
                g7 = (unsigned short)f3((int)2ull, (unsigned char)65535ull, (int)49ull, (signed char)128ull);
            } else {
                g4 = (long long)f1((unsigned long long)f1((unsigned long long)((unsigned long long)(short)f2((short)m3, (unsigned long long)210ull, (long long)0ull) << ((unsigned)(unsigned int)((unsigned int)(long long)g8 + (unsigned int)(signed char)128ull) & 63)), (unsigned long long)((!((unsigned char)ga1[2] >= (short)ga0[0]))), (int)((unsigned int)(unsigned short)(~(unsigned int)(unsigned int)ga0[1]) * (unsigned int)(short)117ull)), (unsigned long long)((unsigned long long)(short)(((unsigned char)((unsigned int)0 - (unsigned int)(short)m3) >= (unsigned long long)f3((int)g8, (unsigned char)g3, (int)g7, (signed char)m0)) ? (short)((unsigned int)(short)m1 + (unsigned int)(unsigned char)1ull) : (long long)((unsigned long long)(int)15ull + (unsigned long long)(long long)g5)) / ((unsigned long long)(long long)f0((short)((unsigned int)(signed char)48ull & (unsigned int)(signed char)g8)) | 1u)), (int)((unsigned int)(int)((((short)g6) || ((unsigned int)g0 >= (unsigned int)m3))) + (unsigned int)(unsigned short)((((unsigned int)g2) && ((unsigned char)3ull != (signed char)ga1[4])) ? (unsigned short)((unsigned int)(unsigned short)7ull | (unsigned int)(unsigned long long)m1) : (int)((unsigned int)(signed char)g7 - (unsigned int)(long long)15ull))));
                g6 = (int)((unsigned int)(signed char)((unsigned int)(unsigned int)(((long long)(((unsigned int)32767ull == (long long)ga0[2]) ? (unsigned long long)m2 : (signed char)g8) != (unsigned int)m3)) / ((unsigned int)(signed char)(((signed char)(((long long)128ull < (long long)g9)) > (unsigned int)((unsigned int)(unsigned int)m2 | (unsigned int)(unsigned long long)0ull))) | 1u)) * (unsigned int)(unsigned short)(~(unsigned int)(short)(((unsigned char)((unsigned int)(short)100ull / ((unsigned int)(unsigned long long)g8 | 1u))) ? (unsigned char)f1((unsigned long long)0ull, (unsigned long long)1ull, (int)1ull) : (unsigned char)f0((short)ga1[2]))));
            }
            break;
        case 6:
            g4 = (long long)((unsigned long long)(long long)((unsigned long long)(unsigned char)ga1[6] ^ (unsigned long long)(signed char)ga1[3]) & (unsigned long long)(unsigned int)g9);
        case 7:
            if ((short)((unsigned int)(long long)f0((short)15ull) << ((unsigned)(unsigned int)((unsigned int)(short)127ull - (unsigned int)(unsigned int)0ull) & 31)) >= (unsigned short)(~(unsigned int)(unsigned long long)((unsigned long long)(short)g0 >> ((unsigned)(unsigned int)ga1[4] & 63)))) {
                m3 = (signed char)15ull;
            }
            break;
        default:
            g4 = (long long)(((unsigned int)((unsigned int)(long long)((unsigned long long)(short)((unsigned int)(unsigned int)g1 << ((unsigned)(unsigned int)g5 & 31)) % ((unsigned long long)(signed char)(((((long long)g7 != (short)ga1[1]) || ((unsigned short)ga1[3] > (unsigned int)g9)) || ((unsigned char)m3 <= (long long)g9))) | 1u)) + (unsigned int)(unsigned long long)f0((short)f0((short)128ull))) >= (short)g6) ? (unsigned short)f0((short)((unsigned int)(unsigned long long)((unsigned long long)(long long)2ull + (unsigned long long)(unsigned char)g4) + (unsigned int)(short)(~(unsigned int)(short)g9))) : (signed char)((unsigned int)(unsigned short)((unsigned int)0 - (unsigned int)(short)(((unsigned int)g7 < (signed char)g2) ? (unsigned short)ga0[2] : (long long)100ull)) & (unsigned int)(unsigned int)((unsigned int)(unsigned char)((unsigned int)(unsigned long long)3ull % ((unsigned int)(unsigned int)44ull | 1u)) * (unsigned int)(unsigned long long)((unsigned long long)(int)ga0[2] & (unsigned long long)(unsigned long long)ga1[5]))));
        }
    case 6:
        g6 = (int)((unsigned int)(short)100ull | (unsigned int)(long long)g3);
        break;
    default:
        g4 = (long long)((unsigned long long)(unsigned long long)((unsigned long long)0 - (unsigned long long)(signed char)((unsigned int)(signed char)((unsigned int)(short)ga1[7] + (unsigned int)(int)g2) / ((unsigned int)(signed char)(((signed char)65535ull < (int)15ull) ? (signed char)127ull : (long long)32767ull) | 1u))) | (unsigned long long)(int)((unsigned int)(int)g5 % ((unsigned int)(signed char)((unsigned int)(unsigned short)255ull * (unsigned int)(signed char)((unsigned int)(long long)0ull & (unsigned int)(short)g7)) | 1u)));
    }
    ga1[7] = (long long)f1((unsigned long long)f2((short)1ull, (unsigned long long)((!((long long)g4 < (signed char)ga0[0]))), (long long)(((((unsigned short)ga1[0] == (unsigned short)1ull) && ((unsigned int)g1)) || (((unsigned short)g1) && (((short)256ull) || ((unsigned char)32767ull == (short)51770ull)))))), (unsigned long long)(((signed char)(~(unsigned int)(unsigned char)m0) >= (unsigned long long)1ull)), (int)g1);
    switch ((int)((int)((unsigned int)(int)m0 + (unsigned int)(unsigned char)(~(unsigned int)(unsigned short)g5))) & 7) {
    case 5:
        m1 = (long long)((unsigned long long)(signed char)168ull | (unsigned long long)(unsigned int)3ull);
        break;
    case 6:
        m1 = (long long)((unsigned long long)(unsigned char)((unsigned int)(signed char)((unsigned int)(unsigned long long)m1 >> ((unsigned)(unsigned int)ga0[2] & 31)) % ((unsigned int)(long long)((unsigned long long)(unsigned long long)g3 >> ((unsigned)(unsigned int)1654609643ull & 63)) | 1u)) ^ (unsigned long long)(int)f0((short)((unsigned int)(signed char)g1 & (unsigned int)(short)2ull)));
    case 7:
        m0 = (signed char)ga1[5];
        break;
    default:
        g2 = (unsigned short)((unsigned int)0 - (unsigned int)(short)g7);
    }
    m3 = (signed char)((unsigned int)(int)(((short)202ull)) ^ (unsigned int)(signed char)((unsigned int)(signed char)m3 - (unsigned int)(long long)2ull));
    m2 = (unsigned short)(((long long)ga0[2] > (unsigned short)ga0[2]));
    m0 = (signed char)1ull;
    m0 = (signed char)(~(unsigned int)(unsigned short)255ull);
    for (int i0 = 0; i0 < 5; i0++) {
        m1 = (long long)((unsigned long long)(unsigned long long)((unsigned long long)(short)f0((short)((unsigned int)(unsigned short)100ull / ((unsigned int)(unsigned int)1ull | 1u))) | (unsigned long long)(short)((unsigned int)(unsigned long long)(((int)32767ull <= (int)g8)) / ((unsigned int)(unsigned char)g1 | 1u))) | (unsigned long long)(unsigned int)(~(unsigned int)(long long)(((((long long)ga0[1] < (unsigned short)g7) || (!((unsigned int)65535ull))) || ((!((unsigned int)32767ull > (unsigned char)g6)) || ((long long)g2 > (short)g0))) ? (unsigned char)((unsigned int)(int)65535ull * (unsigned int)(unsigned char)g8) : (unsigned char)i0)));
    }
    for (int i1 = 0; i1 < 6; i1++) {
        g7 = (unsigned short)((unsigned int)(unsigned short)ga1[4] << ((unsigned)(unsigned int)15ull & 31));
        for (int i2 = 0; i2 < 6; i2++) {
            for (int i3 = 0; i3 < 6; i3++) {
                g8 = (unsigned int)(~(unsigned int)(unsigned char)((unsigned int)(long long)256ull - (unsigned int)(unsigned short)ga1[5]));
            }
            g5 = (unsigned char)(((unsigned int)g9 >= (unsigned short)ga1[3]) ? (long long)m1 : (long long)g8);
        }
    }
    for (int i4 = 0; i4 < 4; i4++) {
        if ((signed char)((unsigned int)(int)((unsigned int)(unsigned short)ga0[1] ^ (unsigned int)(unsigned long long)ga1[3]) % ((unsigned int)(unsigned int)((unsigned int)(int)128ull ^ (unsigned int)(unsigned int)ga1[4]) | 1u)) <= (signed char)((unsigned int)(signed char)((unsigned int)(long long)g7 ^ (unsigned int)(unsigned long long)m1) + (unsigned int)(unsigned int)((unsigned int)(long long)ga0[0] % ((unsigned int)(unsigned char)g8 | 1u)))) {
            { int w5 = 2; while (w5 > 0) {
                g3 = (unsigned short)((unsigned int)(short)((unsigned int)(signed char)(~(unsigned int)(unsigned short)((unsigned int)(unsigned long long)ga0[1] ^ (unsigned int)(unsigned long long)ga0[1])) ^ (unsigned int)(signed char)((unsigned int)(unsigned int)(~(unsigned int)(unsigned long long)m2) + (unsigned int)(unsigned long long)((unsigned long long)(long long)15ull - (unsigned long long)(long long)2ull))) ^ (unsigned int)(long long)((unsigned long long)(unsigned int)((unsigned int)(long long)((unsigned long long)(long long)g0 + (unsigned long long)(unsigned int)ga0[2]) - (unsigned int)(short)((unsigned int)(unsigned long long)7ull >> ((unsigned)(unsigned int)2ull & 31))) >> ((unsigned)(unsigned int)ga0[1] & 63)));
                w5--;
            } }
            g3 = (unsigned short)ga1[0];
            g4 = (long long)((unsigned long long)0 - (unsigned long long)(unsigned int)(((signed char)g3 != (long long)g9) ? (unsigned int)2656103609ull : (unsigned int)g4));
        } else {
            g5 = (unsigned char)32767ull;
        }
        m3 = (signed char)(((long long)((unsigned long long)(signed char)(((unsigned long long)m1 < (unsigned char)ga0[1])) | (unsigned long long)(unsigned short)((unsigned int)(unsigned short)ga0[1] >> ((unsigned)(unsigned int)m1 & 31))) > (long long)((unsigned long long)0 - (unsigned long long)(long long)g0)));
    }
    g7 = (unsigned short)((unsigned int)0 - (unsigned int)(unsigned char)(((short)g4 < (unsigned long long)ga0[2]) ? (unsigned long long)g5 : (signed char)ga1[0]));
    for (int i6 = 0; i6 < 6; i6++) {
        g2 = (unsigned short)128ull;
        g1 = (signed char)g1;
        for (int i7 = 0; i7 < 6; i7++) {
            { int w8 = 4; while (w8 > 0) {
                m1 = (long long)g4;
                w8--;
            } }
            ga0[1] = (unsigned short)32767ull;
        }
    }
    for (int i9 = 0; i9 < 4; i9++) {
        if (!((short)((((unsigned short)ga0[1]) || ((unsigned long long)ga0[1] < (unsigned char)2ull))) != (unsigned char)(~(unsigned int)(long long)i9))) {
            g2 = (unsigned short)((unsigned int)0 - (unsigned int)(long long)3ull);
            g8 = (unsigned int)((unsigned int)(int)m2 % ((unsigned int)(signed char)((unsigned int)(unsigned long long)((((unsigned char)127ull) && ((short)g6)) ? (unsigned long long)g4 : (unsigned int)g6) / ((unsigned int)(unsigned long long)((unsigned long long)(unsigned char)100ull | (unsigned long long)(short)m1) | 1u)) | 1u));
        } else {
            if (((short)((unsigned int)(int)g7 / ((unsigned int)(signed char)g3 | 1u)) <= (long long)((unsigned long long)0 - (unsigned long long)(unsigned int)g4)) && ((unsigned short)f0((short)ga1[6]) <= (unsigned short)ga1[1])) {
                g4 = (long long)((unsigned long long)(unsigned short)127ull - (unsigned long long)(unsigned char)100ull);
                g0 = (long long)((unsigned long long)(short)((((signed char)g9 != (long long)65535ull) || (((long long)1736830933133721054ull < (long long)0ull) || ((long long)g0 != (unsigned char)g8)))) | (unsigned long long)(unsigned long long)((unsigned long long)(long long)ga0[2] ^ (unsigned long long)(unsigned long long)m0));
                g4 = (long long)(((int)128ull < (unsigned long long)((((int)32767ull > (int)g0) || ((unsigned short)g6 <= (unsigned long long)3ull)) ? (unsigned char)1ull : (signed char)g4)) ? (unsigned int)((unsigned int)(unsigned long long)g2 & (unsigned int)(unsigned int)g4) : (signed char)((unsigned int)(long long)128ull % ((unsigned int)(unsigned short)2ull | 1u)));
            } else {
                m2 = (unsigned short)((unsigned int)(unsigned long long)((unsigned long long)(unsigned short)f2((short)m1, (unsigned long long)ga1[7], (long long)g8) | (unsigned long long)(int)90ull) ^ (unsigned int)(unsigned int)((unsigned int)(short)(~(unsigned int)(long long)7ull) * (unsigned int)(unsigned long long)((unsigned long long)(short)65535ull ^ (unsigned long long)(signed char)15ull)));
                g7 = (unsigned short)((unsigned int)(unsigned char)((unsigned int)(unsigned char)((unsigned int)(long long)100ull * (unsigned int)(unsigned short)7ull) & (unsigned int)(short)m1) / ((unsigned int)(unsigned long long)f2((short)((unsigned int)(int)100ull * (unsigned int)(signed char)g7), (unsigned long long)((unsigned long long)(signed char)g2 & (unsigned long long)(unsigned short)m0), (long long)(((long long)m2 >= (signed char)65535ull))) | 1u));
            }
            for (int i10 = 0; i10 < 6; i10++) {
                g4 = (long long)((unsigned long long)(int)g1 >> ((unsigned)(unsigned int)2ull & 63));
                g7 = (unsigned short)((unsigned int)(short)m1 * (unsigned int)(long long)(~(unsigned long long)(unsigned int)m2));
            }
        }
        if ((unsigned long long)f2((short)(((short)255ull == (unsigned short)g8)), (unsigned long long)(((unsigned int)65535ull <= (unsigned short)ga1[7]) ? (unsigned long long)ga0[2] : (int)7ull), (long long)((unsigned long long)(unsigned short)m3 ^ (unsigned long long)(short)ga1[6]))) {
            if ((((((unsigned char)207ull < (unsigned long long)128ull) || (((((int)ga1[4] < (unsigned long long)ga1[6]) || (!((short)65535ull >= (int)127ull))) && ((unsigned short)g1 > (unsigned long long)ga0[2])) || ((unsigned long long)g5 >= (signed char)ga1[4]))) && ((short)128ull == (unsigned int)m2)) && ((long long)128ull != (unsigned long long)g9)) && ((unsigned int)((unsigned int)(unsigned char)g5 << ((unsigned)(unsigned int)g0 & 31)) != (unsigned short)((unsigned int)(short)100ull % ((unsigned int)(unsigned char)100ull | 1u)))) {
                g6 = (int)((unsigned int)(short)((unsigned int)(short)f2((short)15ull, (unsigned long long)ga1[7], (long long)ga1[6]) | (unsigned int)(unsigned char)f0((short)m3)) << ((unsigned)(unsigned int)((unsigned int)(int)((unsigned int)(unsigned int)1ull & (unsigned int)(short)g5) | (unsigned int)(signed char)ga1[5]) & 31));
            }
            m1 = (long long)g9;
        } else {
            g3 = (unsigned short)f2((short)256ull, (unsigned long long)f2((short)((((((long long)g6 == (unsigned int)100ull) || ((signed char)ga1[7] == (unsigned char)m1)) || ((((short)g7 != (unsigned long long)g4) && ((short)g6 >= (int)ga0[0])) && ((((int)ga0[0] == (unsigned int)m3) || (((short)ga0[0] == (unsigned char)g2) && ((int)g9 <= (short)g3))) || ((long long)ga0[2] == (long long)g5)))) || ((unsigned long long)1ull == (long long)65535ull)) ? (short)((unsigned int)(unsigned long long)7200979956042970527ull * (unsigned int)(short)7ull) : (unsigned long long)f1((unsigned long long)100ull, (unsigned long long)1ull, (int)g1)), (unsigned long long)((((int)ga0[2] < (int)g2) && (((((long long)g8 != (unsigned int)ga1[7]) && ((signed char)g9)) && ((unsigned short)ga1[6] >= (unsigned char)65535ull)) && ((unsigned short)65535ull == (unsigned short)g8))) ? (signed char)(~(unsigned int)(unsigned int)ga1[6]) : (unsigned char)(~(unsigned int)(long long)g7)), (long long)((unsigned long long)0 - (unsigned long long)(unsigned long long)((unsigned long long)(unsigned char)256ull - (unsigned long long)(unsigned short)15ull))), (long long)g1);
            switch ((int)((int)(~(unsigned int)(unsigned long long)((((int)g2 > (unsigned long long)0ull) && (((long long)12476463715253769262ull) && ((((unsigned short)2ull > (unsigned short)ga1[4]) && (((unsigned long long)g8 == (unsigned short)255ull) || ((int)65535ull == (unsigned int)g1))) && (((long long)g1 > (unsigned int)g3) && ((signed char)g3 > (unsigned long long)32767ull)))))))) & 7) {
            case 1:
                g1 = (signed char)((unsigned int)(short)((unsigned int)(unsigned int)(~(unsigned int)(unsigned short)((unsigned int)(unsigned long long)m1 / ((unsigned int)(int)i9 | 1u))) | (unsigned int)(unsigned char)m1) + (unsigned int)(short)(~(unsigned int)(signed char)(((unsigned char)(((long long)3ull > (unsigned int)100ull) ? (signed char)g7 : (unsigned short)ga1[3]) == (unsigned long long)((unsigned long long)(int)256ull * (unsigned long long)(unsigned long long)ga0[1])))));
                break;
            case 3:
                g3 = (unsigned short)f0((short)((unsigned int)(unsigned char)g7 - (unsigned int)(long long)((unsigned long long)(unsigned int)32767ull * (unsigned long long)(unsigned char)g0)));
                break;
            default:
                ga0[0] = (unsigned short)((unsigned int)(signed char)15ull - (unsigned int)(signed char)(((unsigned long long)(((short)65535ull != (signed char)m2)) > (unsigned long long)g1) ? (unsigned char)((((unsigned long long)g2 >= (short)32767ull) && (((unsigned char)58ull <= (int)ga0[1]) || ((int)g3 >= (unsigned int)32767ull)))) : (signed char)f2((short)ga0[0], (unsigned long long)ga1[4], (long long)g1)));
            }
        }
        for (int i11 = 0; i11 < 5; i11++) {
            g2 = (unsigned short)27556ull;
        }
    }
    g6 = (int)(~(unsigned int)(int)((((unsigned long long)m3 <= (long long)m1) || (((unsigned char)g2 >= (signed char)128ull) && (((((unsigned short)ga1[5]) || ((unsigned short)7ull <= (long long)255ull)) && ((unsigned int)ga0[1] >= (signed char)ga0[2])) && ((unsigned long long)127ull >= (long long)g7)))) ? (int)4ull : (int)m2));
    if ((short)((unsigned int)0 - (unsigned int)(unsigned long long)((unsigned long long)(unsigned int)g9 / ((unsigned long long)(unsigned short)m2 | 1u))) == (unsigned long long)((((int)100ull != (long long)3ull) && ((unsigned int)g5 == (unsigned short)ga0[2])))) {
        if (((int)((unsigned int)(unsigned long long)g4 | (unsigned int)(unsigned char)256ull) != (short)((!(((unsigned short)g7 < (signed char)g0) && ((int)128ull != (short)ga1[1]))) ? (unsigned int)2ull : (int)32767ull)) || ((unsigned short)127ull <= (signed char)(((int)28ull < (int)ga0[1])))) {
            ga0[1] = (unsigned short)((unsigned int)(unsigned long long)3ull + (unsigned int)(signed char)((unsigned int)(short)((unsigned int)(unsigned long long)ga0[2] << ((unsigned)(unsigned int)15ull & 31)) >> ((unsigned)(unsigned int)((unsigned int)(unsigned char)ga1[3] - (unsigned int)(unsigned short)ga1[5]) & 31)));
            g6 = (int)((unsigned int)(short)100ull ^ (unsigned int)(unsigned int)15ull);
        } else {
            ga1[4] = (long long)((unsigned long long)0 - (unsigned long long)(unsigned long long)f1((unsigned long long)f3((int)ga0[0], (unsigned char)m0, (int)3ull, (signed char)2ull), (unsigned long long)((unsigned long long)(unsigned long long)ga0[1] | (unsigned long long)(unsigned long long)65535ull), (int)((unsigned int)(long long)128ull ^ (unsigned int)(unsigned int)65535ull)));
            { int w12 = 3; while (w12 > 0) {
                m0 = (signed char)((unsigned int)(unsigned long long)((unsigned long long)(unsigned short)7ull / ((unsigned long long)(int)((unsigned int)(unsigned char)((unsigned int)0 - (unsigned int)(int)g8) * (unsigned int)(unsigned short)(~(unsigned int)(short)32767ull)) | 1u)) & (unsigned int)(unsigned short)m1);
                w12--;
            } }
            if ((((short)ga1[1] >= (unsigned long long)m0) || (!((unsigned long long)32767ull > (short)g0))) && (((int)g7 == (unsigned char)g7) && ((unsigned short)0ull < (unsigned long long)255ull))) {
                g0 = (long long)((unsigned long long)(long long)f1((unsigned long long)m2, (unsigned long long)ga1[5], (int)g3) - (unsigned long long)(int)((unsigned int)(int)255ull + (unsigned int)(unsigned short)g0));
                m0 = (signed char)((unsigned int)(unsigned short)((unsigned int)(unsigned short)3ull ^ (unsigned int)(unsigned long long)0ull) >> ((unsigned)(unsigned int)(((unsigned int)m1 >= (unsigned char)g1)) & 31));
                m2 = (unsigned short)(~(unsigned int)(unsigned int)((unsigned int)(unsigned short)(((unsigned long long)((unsigned long long)(short)m3 >> ((unsigned)(unsigned int)3ull & 63)) == (unsigned char)((unsigned int)(unsigned short)100ull / ((unsigned int)(unsigned int)g2 | 1u)))) & (unsigned int)(unsigned long long)((unsigned long long)(unsigned int)((unsigned int)(short)m0 ^ (unsigned int)(int)65535ull) - (unsigned long long)(int)((unsigned int)(short)15ull % ((unsigned int)(unsigned short)m1 | 1u)))));
            } else {
                g9 = (unsigned int)((unsigned int)(unsigned long long)m1 & (unsigned int)(unsigned long long)m1);
                g5 = (unsigned char)((unsigned int)(signed char)g2 & (unsigned int)(unsigned short)ga0[0]);
            }
        }
        for (int i13 = 0; i13 < 2; i13++) {
            switch ((int)((int)m2) & 7) {
            case 0:
                m0 = (signed char)((unsigned int)(unsigned short)g2 & (unsigned int)(unsigned int)((unsigned int)(long long)15ull >> ((unsigned)(unsigned int)(~(unsigned int)(long long)((unsigned long long)(unsigned int)ga0[1] | (unsigned long long)(unsigned int)65535ull)) & 31)));
                break;
            case 2:
                g1 = (signed char)f3((int)100ull, (unsigned char)0ull, (int)g9, (signed char)15ull);
                break;
            case 5:
                m3 = (signed char)((unsigned int)(unsigned int)f2((short)((unsigned int)(unsigned long long)g2 >> ((unsigned)(unsigned int)g0 & 31)), (unsigned long long)3ull, (long long)((unsigned long long)(unsigned short)ga0[0] - (unsigned long long)(unsigned long long)m2)) * (unsigned int)(long long)((unsigned long long)(unsigned int)((unsigned int)(short)3ull ^ (unsigned int)(unsigned long long)3ull) >> ((unsigned)(unsigned int)((unsigned int)(unsigned short)ga0[2] * (unsigned int)(int)i13) & 63)));
                break;
            default:
                ga1[0] = (long long)((unsigned long long)(unsigned int)256ull ^ (unsigned long long)(unsigned long long)((unsigned long long)(short)((unsigned int)(short)g1 - (unsigned int)(long long)g2) ^ (unsigned long long)(unsigned char)256ull));
            }
            for (int i14 = 0; i14 < 1; i14++) {
                g9 = (unsigned int)((unsigned int)(short)(~(unsigned int)(long long)(~(unsigned long long)(unsigned int)(((unsigned long long)g7 <= (unsigned int)ga0[0]) ? (unsigned int)g2 : (long long)ga0[2]))) << ((unsigned)(unsigned int)((((long long)((unsigned long long)(long long)ga0[0] * (unsigned long long)(unsigned char)1ull) < (unsigned short)((unsigned int)(long long)ga0[2] - (unsigned int)(long long)2ull)) && (((unsigned int)ga0[1] <= (long long)g4) && ((unsigned long long)32767ull == (int)g9))) ? (unsigned long long)((unsigned long long)(signed char)m1 ^ (unsigned long long)(signed char)((unsigned int)(short)127ull << ((unsigned)(unsigned int)2ull & 31))) : (short)((unsigned int)(int)((unsigned int)(unsigned short)i14 * (unsigned int)(short)m3) | (unsigned int)(unsigned char)((unsigned int)(long long)15ull / ((unsigned int)(unsigned int)ga0[2] | 1u)))) & 31));
                g6 = (int)f2((short)((unsigned int)(unsigned short)g9 << ((unsigned)(unsigned int)ga0[1] & 31)), (unsigned long long)0ull, (long long)((unsigned long long)(signed char)g7 / ((unsigned long long)(long long)g6 | 1u)));
            }
            g8 = (unsigned int)f3((int)(~(unsigned int)(unsigned char)((unsigned int)(unsigned long long)g0 + (unsigned int)(unsigned short)g1)), (unsigned char)((unsigned int)(unsigned char)((unsigned int)(unsigned long long)g7 + (unsigned int)(int)255ull) | (unsigned int)(unsigned int)(~(unsigned int)(short)g9)), (int)f2((short)1226ull, (unsigned long long)((unsigned long long)(short)ga1[1] + (unsigned long long)(unsigned int)ga1[7]), (long long)f0((short)ga1[7])), (signed char)((unsigned int)0 - (unsigned int)(signed char)(((signed char)61ull <= (signed char)128ull))));
        }
        g5 = (unsigned char)ga0[0];
    }
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
    runtime.printf("m0=%llx\n", (unsigned long long)m0);
    runtime.printf("m1=%llx\n", (unsigned long long)m1);
    runtime.printf("m2=%llx\n", (unsigned long long)m2);
    runtime.printf("m3=%llx\n", (unsigned long long)m3);
    runtime.printf("ga0_0=%llx\n", (unsigned long long)ga0[0]);
    runtime.printf("ga0_1=%llx\n", (unsigned long long)ga0[1]);
    runtime.printf("ga0_2=%llx\n", (unsigned long long)ga0[2]);
    runtime.printf("ga1_0=%llx\n", (unsigned long long)ga1[0]);
    runtime.printf("ga1_1=%llx\n", (unsigned long long)ga1[1]);
    runtime.printf("ga1_2=%llx\n", (unsigned long long)ga1[2]);
    runtime.printf("ga1_3=%llx\n", (unsigned long long)ga1[3]);
    runtime.printf("ga1_4=%llx\n", (unsigned long long)ga1[4]);
    runtime.printf("ga1_5=%llx\n", (unsigned long long)ga1[5]);
    runtime.printf("ga1_6=%llx\n", (unsigned long long)ga1[6]);
    runtime.printf("ga1_7=%llx\n", (unsigned long long)ga1[7]);
    return 0;
}

