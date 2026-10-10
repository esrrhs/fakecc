package main;
import runtime;
unsigned long long g0 = (unsigned long long)255ull;
unsigned int g1 = (unsigned int)128ull;
long long g2 = (long long)255ull;
unsigned long long g3 = (unsigned long long)17728170784492242215ull;
signed char g4 = (signed char)15ull;
short g5 = (short)98ull;
short g6 = (short)3ull;
unsigned long long g7 = (unsigned long long)3ull;
signed char g8 = (signed char)0ull;
unsigned int g9 = (unsigned int)32767ull;
int ga0[8] = {(int)3141981803ull, (int)7ull, (int)1500202434ull, (int)15ull, (int)1ull, (int)100ull, (int)2850122399ull, (int)1142988234ull};
unsigned int ga1[4] = {(unsigned int)779646926ull, (unsigned int)1ull, (unsigned int)1ull, (unsigned int)32767ull};
static short f0(signed char p0, signed char p1, long long p2, short p3) {
    long long l0 = (long long)((unsigned long long)(signed char)((unsigned int)(long long)0ull % ((unsigned int)(long long)ga0[0] | 1u)) * (unsigned long long)(unsigned long long)((unsigned long long)(signed char)g8 % ((unsigned long long)(long long)g5 | 1u)));
    p3 = (short)((unsigned int)(signed char)((unsigned int)(unsigned short)((unsigned int)(int)((unsigned int)(short)g3 & (unsigned int)(long long)3ull) ^ (unsigned int)(unsigned short)(~(unsigned int)(unsigned char)7ull)) + (unsigned int)(unsigned long long)3ull) / ((unsigned int)(int)(((unsigned short)(~(unsigned int)(long long)(((int)g7 != (signed char)100ull))) >= (long long)(~(unsigned long long)(short)((unsigned int)(unsigned long long)32767ull & (unsigned int)(unsigned int)256ull)))) | 1u));
    g1 = (unsigned int)3ull;
    g8 = (signed char)((unsigned int)(int)((unsigned int)(unsigned int)((unsigned int)(unsigned int)((unsigned int)(signed char)65535ull & (unsigned int)(int)128ull) * (unsigned int)(unsigned char)((unsigned int)(unsigned int)p0 - (unsigned int)(int)g8)) * (unsigned int)(unsigned short)((unsigned int)0 - (unsigned int)(long long)((unsigned long long)(short)127ull + (unsigned long long)(unsigned long long)2ull))) ^ (unsigned int)(long long)p0);
    return (short)((unsigned int)(int)(((long long)ga1[1] == (unsigned short)g0)) << ((unsigned)(unsigned int)((unsigned int)(int)ga0[6] | (unsigned int)(unsigned char)g4) & 31));
}

static unsigned short f1(unsigned long long p0) {
    signed char l0 = (signed char)g8;
    { int w0 = 4; while (w0 > 0) {
        g1 = (unsigned int)((unsigned int)(short)((unsigned int)0 - (unsigned int)(unsigned int)((unsigned int)(int)((((unsigned long long)w0 <= (unsigned long long)2ull) && ((long long)ga0[2] <= (signed char)15ull))) + (unsigned int)(signed char)(((unsigned short)g0)))) / ((unsigned int)(short)((unsigned int)(int)((unsigned int)(unsigned long long)3ull | (unsigned int)(long long)94ull) - (unsigned int)(unsigned short)((unsigned int)(unsigned char)((unsigned int)(int)p0 ^ (unsigned int)(short)1ull) >> ((unsigned)(unsigned int)((unsigned int)(long long)65535ull + (unsigned int)(int)p0) & 31))) | 1u));
        w0--;
    } }
    switch ((int)((int)((unsigned int)(int)7ull / ((unsigned int)(signed char)((unsigned int)(long long)7ull >> ((unsigned)(unsigned int)ga1[2] & 31)) | 1u))) & 7) {
    case 0:
        if ((signed char)(((!((long long)g7 < (long long)ga1[2])) || ((long long)ga0[6]))) == (signed char)((unsigned int)(int)g0 / ((unsigned int)(short)ga0[0] | 1u))) {
            ga0[6] = (int)f0((signed char)((unsigned int)(signed char)((unsigned int)(int)g6 >> ((unsigned)(unsigned int)g8 & 31)) * (unsigned int)(int)((unsigned int)(short)ga0[5] + (unsigned int)(signed char)2ull)), (signed char)((unsigned int)(int)(((unsigned char)32767ull == (unsigned short)ga0[6]) ? (unsigned long long)p0 : (short)g1) - (unsigned int)(short)f0((signed char)32767ull, (signed char)7ull, (long long)128ull, (short)33333ull)), (long long)(~(unsigned long long)(unsigned int)((unsigned int)(signed char)ga1[0] % ((unsigned int)(unsigned int)g2 | 1u))), (short)((unsigned int)(unsigned char)g7 << ((unsigned)(unsigned int)((unsigned int)(short)255ull - (unsigned int)(long long)g0) & 31)));
        }
        break;
    case 2:
        for (int i1 = 0; i1 < 2; i1++) {
            ga0[7] = (int)((unsigned int)(unsigned long long)((unsigned long long)(unsigned char)127ull >> ((unsigned)(unsigned int)(((signed char)ga0[6]) ? (unsigned int)65535ull : (short)15ull) & 63)) & (unsigned int)(long long)65535ull);
            g7 = (unsigned long long)((unsigned long long)(signed char)((unsigned int)(long long)(~(unsigned long long)(unsigned short)ga0[5]) / ((unsigned int)(unsigned char)((unsigned int)(unsigned char)p0 + (unsigned int)(short)g8) | 1u)) % ((unsigned long long)(unsigned long long)((unsigned long long)(signed char)f0((signed char)g6, (signed char)ga0[2], (long long)3ull, (short)100ull) % ((unsigned long long)(int)((unsigned int)(short)g1 % ((unsigned int)(unsigned char)ga1[0] | 1u)) | 1u)) | 1u));
        }
        break;
    case 4:
        g0 = (unsigned long long)((unsigned long long)(long long)((unsigned long long)(int)l0 % ((unsigned long long)(short)ga1[1] | 1u)) % ((unsigned long long)(signed char)((unsigned int)(unsigned char)g3 | (unsigned int)(int)128ull) | 1u));
    case 5:
        if ((((unsigned int)g3 < (unsigned long long)ga0[7]) && ((unsigned char)ga0[4] < (short)g7)) && ((((((int)g7 == (signed char)ga1[2]) && ((((unsigned char)140ull != (unsigned int)ga0[6]) || (((unsigned char)68ull <= (unsigned int)g4) || ((unsigned long long)g9 <= (unsigned long long)3ull))) && ((unsigned short)7ull > (signed char)g7))) || (!((((short)255ull <= (int)3ull) || ((unsigned int)ga0[1] != (unsigned char)256ull)) || ((signed char)15ull > (unsigned long long)ga1[2])))) && (((unsigned short)65535ull < (unsigned char)2ull) && ((unsigned long long)g5 <= (unsigned short)77ull))) || ((unsigned char)l0 <= (int)65535ull))) {
            g6 = (short)ga0[4];
            g5 = (short)((unsigned int)(unsigned long long)g0 + (unsigned int)(unsigned char)g0);
        }
        break;
    default:
        l0 = (signed char)((unsigned int)(signed char)((unsigned int)(signed char)0ull - (unsigned int)(unsigned int)255ull) * (unsigned int)(unsigned long long)((unsigned long long)(unsigned short)l0 | (unsigned long long)(unsigned short)ga1[1]));
    }
    return (unsigned short)(~(unsigned int)(long long)((unsigned long long)(long long)g7 & (unsigned long long)(unsigned long long)g2));
}

static int f2(int p0, long long p1, signed char p2, unsigned int p3) {
    unsigned char l0 = (unsigned char)100ull;
    g5 = (short)(((int)(~(unsigned int)(long long)g7) <= (unsigned char)((unsigned int)0 - (unsigned int)(unsigned int)255ull)));
    if ((unsigned int)128ull) {
        if ((long long)f0((signed char)32767ull, (signed char)(((short)15ull) ? (unsigned char)15ull : (unsigned int)128ull), (long long)((((unsigned long long)1ull >= (unsigned long long)7ull) || (((short)180ull <= (unsigned long long)32767ull) || (((unsigned int)g8 >= (unsigned int)0ull) && (!((signed char)100ull < (unsigned int)100ull)))))), (short)((unsigned int)(long long)p2 - (unsigned int)(unsigned char)g8))) {
            p2 = (signed char)((((unsigned long long)((unsigned long long)(long long)g1 + (unsigned long long)(unsigned char)g4) != (signed char)((unsigned int)(unsigned int)g6 & (unsigned int)(unsigned short)0ull)) && (((signed char)ga0[3] != (long long)ga0[5]) && ((short)g4 >= (unsigned int)ga0[1]))));
            p0 = (int)128ull;
        } else {
            g0 = (unsigned long long)((unsigned long long)0 - (unsigned long long)(int)((unsigned int)(short)g7 & (unsigned int)(unsigned char)f1((unsigned long long)((unsigned long long)(unsigned int)32767ull ^ (unsigned long long)(unsigned char)127ull))));
        }
        ga1[0] = (unsigned int)((unsigned int)(unsigned short)((unsigned int)0 - (unsigned int)(unsigned short)((unsigned int)(unsigned char)p3 | (unsigned int)(unsigned long long)ga0[0])) | (unsigned int)(int)((unsigned int)(unsigned short)f1((unsigned long long)7ull) % ((unsigned int)(unsigned short)ga1[3] | 1u)));
        g9 = (unsigned int)(((unsigned long long)((unsigned long long)(signed char)ga0[0] + (unsigned long long)(unsigned int)ga1[1]) >= (unsigned short)((unsigned int)(short)ga0[2] + (unsigned int)(signed char)1ull)) ? (long long)(~(unsigned long long)(signed char)g0) : (signed char)p2);
    }
    g9 = (unsigned int)((!((short)128ull == (unsigned int)((unsigned int)(unsigned int)256ull << ((unsigned)(unsigned int)g2 & 31)))));
    for (int i0 = 0; i0 < 2; i0++) {
        g9 = (unsigned int)(((unsigned char)7ull >= (unsigned int)(~(unsigned int)(long long)128ull)));
        { int w1 = 5; while (w1 > 0) {
            ga1[3] = (unsigned int)((unsigned int)(signed char)((((!((unsigned int)ga1[3] > (long long)128ull)) && (!(!((unsigned short)3ull < (long long)ga0[7])))) && ((unsigned long long)i0 != (unsigned short)ga0[6]))) + (unsigned int)(int)((unsigned int)(unsigned int)((unsigned int)(signed char)g3 - (unsigned int)(short)42284ull) & (unsigned int)(unsigned char)((unsigned int)(short)ga1[2] * (unsigned int)(short)ga1[3])));
            w1--;
        } }
        for (int i2 = 0; i2 < 6; i2++) {
            g6 = (short)g4;
        }
    }
    if (((int)f0((signed char)g8, (signed char)g5, (long long)3ull, (short)ga1[3])) && ((long long)(((unsigned char)ga1[1] < (unsigned short)ga0[1])) > (unsigned short)((unsigned int)(unsigned long long)g6 % ((unsigned int)(signed char)ga1[2] | 1u)))) {
        if (((int)((unsigned int)(unsigned char)g5 - (unsigned int)(long long)ga0[0]) <= (signed char)(((unsigned char)g5 == (long long)g4))) || ((((((unsigned short)2ull >= (unsigned char)g4) || (!((unsigned short)g5 == (unsigned short)65535ull))) && ((int)g8 > (long long)ga1[2])) && ((signed char)ga1[2])) || ((unsigned char)255ull <= (long long)g0))) {
            g7 = (unsigned long long)((unsigned long long)(long long)((unsigned long long)(unsigned char)128ull * (unsigned long long)(signed char)g9) - (unsigned long long)(int)((unsigned int)(unsigned int)ga1[1] >> ((unsigned)(unsigned int)ga0[2] & 31)));
            g2 = (long long)(~(unsigned long long)(signed char)(((int)g1 > (unsigned int)p1) ? (signed char)l0 : (short)((unsigned int)(unsigned int)100ull << ((unsigned)(unsigned int)l0 & 31))));
            p1 = (long long)((unsigned long long)(long long)((!(((unsigned long long)ga0[2] != (unsigned char)p2) && ((unsigned int)g7 < (long long)ga1[0]))) ? (unsigned char)((!(((((int)g0 > (int)58ull) || ((unsigned long long)255ull)) || ((((((((signed char)100ull == (short)7ull) || (((unsigned short)p1 == (unsigned int)65535ull) && ((int)p0 > (short)15ull))) && ((unsigned char)15ull < (long long)7ull)) || ((unsigned long long)g0 > (unsigned char)g9)) || ((unsigned short)g4 <= (signed char)g3)) && ((unsigned short)ga0[7] > (unsigned int)97ull)) && ((unsigned short)256ull <= (unsigned int)ga1[1]))) || ((signed char)1ull)))) : (signed char)((unsigned int)(short)255ull - (unsigned int)(short)p0)) - (unsigned long long)(unsigned long long)((unsigned long long)(unsigned long long)(((int)g8 <= (short)g8)) % ((unsigned long long)(signed char)((unsigned int)(unsigned char)76ull >> ((unsigned)(unsigned int)32767ull & 31)) | 1u)));
        } else {
            p0 = (int)l0;
        }
        ga1[3] = (unsigned int)f1((unsigned long long)((unsigned long long)(unsigned int)(~(unsigned int)(long long)g9) * (unsigned long long)(unsigned short)((unsigned int)(unsigned short)g6 - (unsigned int)(unsigned char)234ull)));
    } else {
        p0 = (int)((unsigned int)(unsigned int)g3 / ((unsigned int)(int)((unsigned int)(signed char)ga1[3] << ((unsigned)(unsigned int)((unsigned int)(int)p2 - (unsigned int)(signed char)((unsigned int)(unsigned long long)g2 | (unsigned int)(unsigned char)7ull)) & 31)) | 1u));
        g6 = (short)((unsigned int)0 - (unsigned int)(long long)f0((signed char)g5, (signed char)((unsigned int)(unsigned long long)g2 & (unsigned int)(unsigned long long)256ull), (long long)p1, (short)f0((signed char)32767ull, (signed char)100ull, (long long)p0, (short)32767ull)));
    }
    g3 = (unsigned long long)(((unsigned char)(((short)33308ull > (long long)l0))) ? (unsigned long long)g7 : (unsigned short)g4);
    return (int)(((unsigned short)g4) ? (signed char)((unsigned int)(long long)15133770579433709602ull | (unsigned int)(unsigned char)g1) : (unsigned short)((unsigned int)(short)g7 ^ (unsigned int)(unsigned long long)p1));
}

int main(void) {
    long long m0 = (long long)256ull;
    unsigned long long m1 = (unsigned long long)7820150245013289205ull;
    if (((unsigned int)ga0[1] != (unsigned long long)m0) || ((unsigned short)((unsigned int)(unsigned long long)g8 * (unsigned int)(long long)ga1[2]) > (unsigned short)((unsigned int)(int)g1 | (unsigned int)(int)ga0[5]))) {
        m1 = (unsigned long long)((unsigned long long)(signed char)(~(unsigned int)(unsigned long long)g0) | (unsigned long long)(unsigned int)((unsigned int)(signed char)256ull >> ((unsigned)(unsigned int)g8 & 31)));
        switch ((int)((int)((unsigned int)(unsigned int)((unsigned int)(unsigned char)3ull | (unsigned int)(unsigned int)ga1[1]) << ((unsigned)(unsigned int)f2((int)0ull, (long long)7ull, (signed char)g3, (unsigned int)g1) & 31))) & 7) {
        case 0:
            g5 = (short)g3;
            break;
        case 1:
            g1 = (unsigned int)((unsigned int)(unsigned short)65535ull - (unsigned int)(unsigned long long)7ull);
            break;
        case 5:
            ga0[6] = (int)((unsigned int)(unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(short)255ull + (unsigned long long)(unsigned short)g0) >> ((unsigned)(unsigned int)((unsigned int)(unsigned long long)g5 >> ((unsigned)(unsigned int)ga0[1] & 31)) & 31)) & (unsigned int)(unsigned short)((unsigned int)(unsigned int)ga0[5] | (unsigned int)(signed char)127ull));
        default:
            for (int i0 = 0; i0 < 5; i0++) {
                g1 = (unsigned int)3ull;
                ga1[1] = (unsigned int)((!((unsigned char)255ull <= (long long)(~(unsigned long long)(unsigned int)128ull))) ? (unsigned short)((unsigned int)(unsigned int)((unsigned int)(signed char)ga1[0] ^ (unsigned int)(signed char)ga1[1]) >> ((unsigned)(unsigned int)((unsigned int)(unsigned char)g3 - (unsigned int)(unsigned char)g4) & 31)) : (int)(~(unsigned int)(short)15ull));
                g8 = (signed char)((unsigned int)(unsigned char)((unsigned int)(short)ga1[0] + (unsigned int)(unsigned char)i0) - (unsigned int)(int)((unsigned int)(unsigned short)g0 | (unsigned int)(short)ga0[3]));
            }
        }
        if ((unsigned int)((unsigned int)(short)((unsigned int)(unsigned int)g7 / ((unsigned int)(short)ga1[2] | 1u)) % ((unsigned int)(unsigned long long)((unsigned long long)(long long)1ull - (unsigned long long)(short)ga0[1]) | 1u)) <= (unsigned long long)255ull) {
            for (int i1 = 0; i1 < 6; i1++) {
                g8 = (signed char)g9;
                g9 = (unsigned int)((((unsigned char)g5 >= (unsigned char)g6) && ((unsigned short)m0 != (unsigned short)g3)) ? (unsigned int)i1 : (long long)255ull);
                ga0[0] = (int)((unsigned int)(long long)128ull | (unsigned int)(short)((unsigned int)(unsigned int)g6 / ((unsigned int)(int)(~(unsigned int)(int)146641229ull) | 1u)));
            }
            ga1[2] = (unsigned int)(~(unsigned int)(unsigned int)(((unsigned short)((unsigned int)0 - (unsigned int)(long long)ga1[1]))));
        }
    } else {
        g7 = (unsigned long long)((unsigned long long)(long long)((unsigned long long)(signed char)((unsigned int)(long long)15ull - (unsigned int)(signed char)7ull) ^ (unsigned long long)(unsigned char)f1((unsigned long long)1ull)) - (unsigned long long)(short)((unsigned int)(unsigned int)ga0[4] | (unsigned int)(unsigned int)(((unsigned short)m0 >= (unsigned int)g8))));
    }
    m0 = (long long)((unsigned long long)(unsigned short)((unsigned int)(unsigned int)g6 ^ (unsigned int)(unsigned char)ga1[2]) - (unsigned long long)(unsigned long long)((unsigned long long)(unsigned int)2ull | (unsigned long long)(unsigned short)ga1[0]));
    switch ((int)((int)((unsigned int)(unsigned char)((unsigned int)(unsigned int)15ull << ((unsigned)(unsigned int)ga0[2] & 31)) & (unsigned int)(short)((unsigned int)(unsigned short)ga0[2] - (unsigned int)(unsigned int)g2))) & 7) {
    case 0:
        { int w2 = 1; while (w2 > 0) {
            switch ((int)((int)g2) & 7) {
            case 1:
                g7 = (unsigned long long)(~(unsigned long long)(int)((unsigned int)(unsigned int)((unsigned int)(unsigned char)m1 * (unsigned int)(unsigned int)w2) >> ((unsigned)(unsigned int)((unsigned int)(short)w2 * (unsigned int)(unsigned char)ga1[1]) & 31)));
            case 3:
                g2 = (long long)((unsigned long long)(short)165ull | (unsigned long long)(signed char)g1);
            case 6:
                g0 = (unsigned long long)((unsigned long long)0 - (unsigned long long)(long long)127ull);
                break;
            case 7:
                g2 = (long long)((unsigned long long)(int)((unsigned int)(long long)g0 - (unsigned int)(unsigned char)((unsigned int)(unsigned short)(~(unsigned int)(int)g9) - (unsigned int)(unsigned int)((unsigned int)(unsigned short)g2 ^ (unsigned int)(signed char)ga1[0]))) & (unsigned long long)(short)((unsigned int)(int)((((short)g6 <= (signed char)2ull) && ((unsigned int)ga0[2])) ? (unsigned int)100ull : (unsigned char)(((((signed char)ga0[2] > (unsigned int)127ull) || ((unsigned char)m1)) || (((long long)w2 < (unsigned int)ga1[2]) || (((unsigned long long)128ull <= (signed char)g8) && (((unsigned int)ga1[2] > (unsigned short)21415ull) || ((short)ga1[1]))))))) / ((unsigned int)(short)g3 | 1u)));
            default:
                g5 = (short)((unsigned int)(unsigned long long)g7 & (unsigned int)(int)g0);
            }
            w2--;
        } }
    case 3:
        { int w3 = 2; while (w3 > 0) {
            for (int i4 = 0; i4 < 4; i4++) {
                g5 = (short)g7;
            }
            w3--;
        } }
        break;
    case 6:
        m1 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)(int)((unsigned int)(long long)m0 / ((unsigned int)(unsigned short)g8 | 1u)) & (unsigned long long)(signed char)((unsigned int)(signed char)((unsigned int)(signed char)m1 & (unsigned int)(unsigned int)g5) & (unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(int)ga1[2]))) + (unsigned long long)(long long)(~(unsigned long long)(unsigned int)((((long long)g1) && ((short)ga1[2] <= (long long)g0)) ? (unsigned int)((unsigned int)(int)15ull * (unsigned int)(unsigned char)m0) : (signed char)((unsigned int)(long long)ga0[5] | (unsigned int)(signed char)216ull))));
    default:
        if ((((unsigned int)m1 <= (unsigned long long)g9) || ((((long long)0ull) || ((((long long)g9 > (unsigned long long)g9) || ((int)g7 >= (long long)128ull)) && ((unsigned long long)g2 >= (unsigned short)g9))) || (((unsigned int)0ull <= (int)ga1[0]) && ((unsigned short)127ull)))) || ((unsigned long long)(~(unsigned long long)(unsigned short)ga1[3]) == (unsigned short)((!(((unsigned short)m0 < (long long)7ull) || ((long long)g7 > (unsigned short)7ull)))))) {
            if (((signed char)((unsigned int)(unsigned int)15ull | (unsigned int)(unsigned short)100ull) < (signed char)((unsigned int)(unsigned long long)g3 & (unsigned int)(int)ga0[4])) && ((unsigned short)m1 >= (short)(((!((long long)m0 <= (signed char)g4)) && ((signed char)128ull > (unsigned int)3ull)) ? (unsigned char)100ull : (unsigned char)g4))) {
                g8 = (signed char)((unsigned int)(unsigned int)((unsigned int)(signed char)3ull << ((unsigned)(unsigned int)96ull & 31)) * (unsigned int)(int)((unsigned int)0 - (unsigned int)(short)ga0[3]));
                g6 = (short)((unsigned int)(short)((unsigned int)(long long)ga0[3] << ((unsigned)(unsigned int)15ull & 31)) ^ (unsigned int)(long long)((unsigned long long)0 - (unsigned long long)(int)ga0[4]));
            } else {
                g7 = (unsigned long long)((!((unsigned long long)g8 != (unsigned int)f1((unsigned long long)g4))) ? (long long)0ull : (unsigned char)(~(unsigned int)(unsigned int)((unsigned int)(unsigned int)256ull * (unsigned int)(unsigned short)15ull)));
                g7 = (unsigned long long)65535ull;
            }
            switch ((int)((int)m1) & 7) {
            case 5:
                g6 = (short)(~(unsigned int)(int)ga1[1]);
                break;
            case 6:
                g1 = (unsigned int)((unsigned int)(unsigned char)(((((unsigned char)g0 >= (long long)g5) && (((long long)ga1[1] <= (int)ga1[2]) && ((signed char)128ull < (unsigned short)ga0[6]))) && ((short)ga1[1] >= (short)ga1[1]))) ^ (unsigned int)(signed char)((unsigned int)(int)((unsigned int)(long long)18027629607977772606ull >> ((unsigned)(unsigned int)g3 & 31)) >> ((unsigned)(unsigned int)m0 & 31)));
                break;
            default:
                ga1[2] = (unsigned int)(((unsigned short)ga1[1] != (long long)g9));
            }
        }
    }
    switch ((int)((int)(~(unsigned int)(unsigned int)((((unsigned char)g8 >= (int)g0) && ((!(!(!((unsigned long long)g0)))) && ((signed char)g9)))))) & 7) {
    case 3:
        for (int i5 = 0; i5 < 1; i5++) {
            if ((unsigned int)((unsigned int)0 - (unsigned int)(int)f0((signed char)65535ull, (signed char)ga1[0], (long long)g0, (short)ga1[0])) < (unsigned long long)(~(unsigned long long)(unsigned long long)((unsigned long long)(unsigned short)ga0[1] >> ((unsigned)(unsigned int)g1 & 63)))) {
                g5 = (short)f0((signed char)((unsigned int)(unsigned char)3ull | (unsigned int)(signed char)7ull), (signed char)(((((int)g0 >= (long long)15ull) || (((unsigned int)1ull <= (long long)ga1[2]) || ((signed char)ga0[4] <= (unsigned long long)2ull))) || (((signed char)ga0[0] > (signed char)g9) || ((short)ga1[2] > (int)3ull))) ? (signed char)7ull : (long long)8901701237834329992ull), (long long)((unsigned long long)(unsigned short)ga1[2] / ((unsigned long long)(unsigned long long)ga1[2] | 1u)), (short)((unsigned int)(int)255ull % ((unsigned int)(unsigned long long)g5 | 1u)));
                g4 = (signed char)((unsigned int)(int)((((unsigned int)128ull != (unsigned int)61ull) && ((unsigned short)g6 >= (unsigned char)89ull))) ^ (unsigned int)(unsigned long long)(~(unsigned long long)(int)((unsigned int)(signed char)g6 + (unsigned int)(unsigned short)128ull)));
                g9 = (unsigned int)((unsigned int)(unsigned short)((unsigned int)(unsigned char)65535ull - (unsigned int)(unsigned long long)g0) >> ((unsigned)(unsigned int)(((int)ga0[6] != (unsigned int)g7) ? (unsigned char)g9 : (unsigned short)m0) & 31));
            }
        }
        break;
    case 7:
        switch ((int)((int)((unsigned int)(unsigned long long)(~(unsigned long long)(int)g9) + (unsigned int)(unsigned long long)m0)) & 7) {
        case 3:
            { int w6 = 2; while (w6 > 0) {
                g1 = (unsigned int)(((unsigned short)((unsigned int)(int)((unsigned int)(unsigned short)((unsigned int)0 - (unsigned int)(short)g7) << ((unsigned)(unsigned int)((unsigned int)(unsigned short)g6 * (unsigned int)(short)g4) & 31)) / ((unsigned int)(unsigned char)(~(unsigned int)(short)(((((int)ga0[0] != (unsigned int)g6) || ((int)255ull)) || ((unsigned int)ga0[6] > (short)g1)))) | 1u)) == (unsigned long long)(~(unsigned long long)(unsigned short)128ull)));
                w6--;
            } }
            break;
        case 5:
            if ((short)((unsigned int)(long long)165ull % ((unsigned int)(short)(~(unsigned int)(int)3ull) | 1u)) != (unsigned short)((unsigned int)(unsigned char)((unsigned int)(unsigned int)g8 - (unsigned int)(unsigned long long)ga1[1]) >> ((unsigned)(unsigned int)((unsigned int)(unsigned int)g8 + (unsigned int)(unsigned long long)g1) & 31))) {
                g9 = (unsigned int)f1((unsigned long long)((unsigned long long)(short)(~(unsigned int)(long long)((unsigned long long)(signed char)15ull % ((unsigned long long)(signed char)m0 | 1u))) | (unsigned long long)(unsigned long long)(((int)((unsigned int)(signed char)0ull * (unsigned int)(unsigned short)100ull) > (int)((unsigned int)(unsigned short)g6 >> ((unsigned)(unsigned int)3ull & 31))) ? (unsigned long long)((unsigned long long)(unsigned int)g3 & (unsigned long long)(unsigned int)256ull) : (long long)127ull)));
            } else {
                g9 = (unsigned int)255ull;
                g1 = (unsigned int)(((unsigned long long)1ull > (unsigned char)3ull) ? (signed char)m1 : (long long)g3);
                g6 = (short)((unsigned int)(unsigned int)((unsigned int)(unsigned short)g9 ^ (unsigned int)(unsigned char)g5) * (unsigned int)(int)((unsigned int)(unsigned long long)m1 | (unsigned int)(unsigned char)256ull));
            }
        case 6:
            g6 = (short)256ull;
            break;
        case 7:
            ga1[3] = (unsigned int)((unsigned int)(unsigned char)(~(unsigned int)(unsigned int)(((int)g3 <= (unsigned long long)m0) ? (signed char)2ull : (unsigned long long)g0)) ^ (unsigned int)(signed char)m1);
            break;
        default:
            g4 = (signed char)((!(((unsigned long long)g9 <= (long long)g7) && ((short)32767ull < (unsigned short)7ull))) ? (signed char)(((unsigned char)((unsigned int)(int)65535ull * (unsigned int)(int)g1) > (unsigned long long)((unsigned long long)(short)3ull >> ((unsigned)(unsigned int)ga1[1] & 63)))) : (int)f1((unsigned long long)((unsigned long long)(signed char)3ull | (unsigned long long)(unsigned short)g6)));
        }
    default:
        for (int i7 = 0; i7 < 6; i7++) {
            { int w8 = 2; while (w8 > 0) {
                g1 = (unsigned int)g6;
                w8--;
            } }
        }
    }
    m1 = (unsigned long long)m0;
    g7 = (unsigned long long)((unsigned long long)(int)ga1[1] & (unsigned long long)(unsigned long long)g8);
    g9 = (unsigned int)((unsigned int)(signed char)1ull & (unsigned int)(unsigned char)ga0[3]);
    g0 = (unsigned long long)((unsigned long long)(int)f0((signed char)f2((int)256ull, (long long)ga0[4], (signed char)g3, (unsigned int)ga0[1]), (signed char)32767ull, (long long)((unsigned long long)(unsigned short)g4 + (unsigned long long)(short)255ull), (short)((unsigned int)(long long)0ull >> ((unsigned)(unsigned int)15ull & 31))) - (unsigned long long)(unsigned char)((unsigned int)(unsigned int)g4 * (unsigned int)(unsigned long long)((unsigned long long)(signed char)3ull + (unsigned long long)(unsigned long long)ga0[0])));
    for (int i9 = 0; i9 < 4; i9++) {
        if (!((long long)((unsigned long long)0 - (unsigned long long)(short)ga0[1]) != (signed char)((unsigned int)(unsigned char)g6 + (unsigned int)(long long)g3))) {
            { int w10 = 2; while (w10 > 0) {
                g9 = (unsigned int)((unsigned int)(int)((unsigned int)(unsigned char)((unsigned int)(unsigned int)0ull >> ((unsigned)(unsigned int)2ull & 31)) * (unsigned int)(signed char)g2) + (unsigned int)(unsigned long long)((((int)g2 >= (unsigned char)ga0[7]) && ((long long)g9 == (unsigned char)ga0[6])) ? (long long)((unsigned long long)(unsigned int)g5 + (unsigned long long)(signed char)1ull) : (unsigned short)(((int)g7 >= (long long)1ull) ? (long long)g8 : (unsigned short)w10)));
                w10--;
            } }
            g3 = (unsigned long long)((unsigned long long)(short)7ull / ((unsigned long long)(long long)((unsigned long long)(short)f0((signed char)ga1[0], (signed char)ga0[6], (long long)65535ull, (short)m0) * (unsigned long long)(short)((unsigned int)(signed char)ga1[2] ^ (unsigned int)(int)ga0[2])) | 1u));
        } else {
            m1 = (unsigned long long)g2;
            g7 = (unsigned long long)((unsigned long long)(unsigned int)((unsigned int)(unsigned short)g8 / ((unsigned int)(unsigned int)ga1[3] | 1u)) - (unsigned long long)(short)(((long long)ga0[6] != (short)m0) ? (short)g9 : (long long)147ull));
            g0 = (unsigned long long)g5;
        }
        switch ((int)((int)(((unsigned long long)(~(unsigned long long)(unsigned int)ga0[3]) > (unsigned char)f2((int)g6, (long long)100ull, (signed char)65535ull, (unsigned int)ga1[3])) ? (signed char)((unsigned int)(unsigned long long)ga1[1] ^ (unsigned int)(unsigned long long)ga1[0]) : (unsigned long long)((unsigned long long)(unsigned int)g2 * (unsigned long long)(unsigned int)ga1[2]))) & 7) {
        case 0:
            for (int i11 = 0; i11 < 6; i11++) {
                g7 = (unsigned long long)((unsigned long long)(unsigned long long)((unsigned long long)0 - (unsigned long long)(int)g8) * (unsigned long long)(unsigned long long)g3);
                g3 = (unsigned long long)(((!(((unsigned char)g1 > (int)g8) && ((((short)m0 < (signed char)100ull) || (((unsigned short)7ull < (long long)g9) && ((unsigned short)12985ull))) && ((((unsigned short)g3 < (int)ga1[3]) || (!((unsigned short)128ull))) && ((unsigned short)m1 <= (short)g2))))) && ((unsigned int)ga1[0] >= (unsigned short)g3)) ? (signed char)2ull : (short)g3);
                g5 = (short)((unsigned int)(unsigned long long)((unsigned long long)(unsigned char)158ull / ((unsigned long long)(signed char)g9 | 1u)) & (unsigned int)(unsigned short)((unsigned int)(unsigned long long)ga1[3] ^ (unsigned int)(short)1ull));
            }
        case 1:
            if ((((unsigned long long)ga0[5] < (signed char)ga1[0]) || (((short)ga1[3] <= (unsigned int)g3) || ((unsigned int)g3 < (unsigned short)32767ull))) || (((unsigned int)100ull) && ((unsigned short)g9 != (unsigned long long)1ull))) {
                g9 = (unsigned int)((unsigned int)(unsigned short)i9 ^ (unsigned int)(unsigned long long)f1((unsigned long long)f2((int)(~(unsigned int)(unsigned int)127ull), (long long)127ull, (signed char)((unsigned int)(short)g8 % ((unsigned int)(long long)g5 | 1u)), (unsigned int)(((unsigned int)ga1[0] != (long long)g2) ? (unsigned long long)g5 : (short)3ull))));
                g3 = (unsigned long long)((unsigned long long)(unsigned char)((unsigned int)(unsigned char)f0((signed char)((unsigned int)(long long)ga0[7] << ((unsigned)(unsigned int)94ull & 31)), (signed char)m0, (long long)ga1[3], (short)((unsigned int)0 - (unsigned int)(long long)ga0[1])) / ((unsigned int)(int)((unsigned int)(unsigned char)((unsigned int)(long long)m1 % ((unsigned int)(int)ga0[3] | 1u)) | (unsigned int)(unsigned short)(((long long)128ull >= (unsigned short)1ull) ? (long long)g2 : (long long)255ull)) | 1u)) + (unsigned long long)(unsigned char)((unsigned int)(long long)183ull + (unsigned int)(unsigned int)((unsigned int)(long long)((unsigned long long)(short)g6 + (unsigned long long)(unsigned int)g1) * (unsigned int)(int)((unsigned int)(signed char)i9 + (unsigned int)(unsigned int)ga0[1]))));
            }
        case 2:
            for (int i12 = 0; i12 < 2; i12++) {
                ga0[2] = (int)f0((signed char)((unsigned int)(long long)((!(((unsigned char)g4 != (unsigned long long)m1) || (((unsigned int)g3 >= (unsigned long long)m1) || (((unsigned char)255ull != (int)ga0[7]) || ((short)127ull)))))) << ((unsigned)(unsigned int)f2((int)2015215088ull, (long long)32767ull, (signed char)g8, (unsigned int)3ull) & 31)), (signed char)f0((signed char)f0((signed char)g3, (signed char)221ull, (long long)100ull, (short)g2), (signed char)((unsigned int)(short)g0 | (unsigned int)(long long)g1), (long long)((((unsigned int)g0 >= (signed char)92ull) && (!((unsigned long long)g1 > (short)g0))) ? (unsigned char)g7 : (unsigned int)ga1[3]), (short)((unsigned int)(unsigned long long)g8 | (unsigned int)(unsigned short)g3)), (long long)((unsigned long long)(signed char)g1 | (unsigned long long)(signed char)((unsigned int)(long long)ga1[0] / ((unsigned int)(unsigned char)ga1[0] | 1u))), (short)((unsigned int)0 - (unsigned int)(int)f1((unsigned long long)256ull)));
            }
            break;
        case 5:
            for (int i13 = 0; i13 < 3; i13++) {
                g0 = (unsigned long long)((unsigned long long)(short)g5 & (unsigned long long)(unsigned char)((unsigned int)(unsigned long long)((unsigned long long)(unsigned long long)ga0[3] + (unsigned long long)(unsigned long long)g9) - (unsigned int)(long long)g1));
                g9 = (unsigned int)((((unsigned int)1ull < (unsigned short)100ull) || ((unsigned char)ga0[1] >= (unsigned char)68ull)) ? (unsigned char)(~(unsigned int)(unsigned long long)g6) : (unsigned char)f0((signed char)g6, (signed char)ga0[2], (long long)ga0[5], (short)m0));
                g2 = (long long)(((((int)g8 == (unsigned short)ga1[0]) && ((unsigned short)ga1[0] == (int)ga1[3])) && ((((unsigned long long)g8 > (unsigned char)g9) || ((unsigned int)ga1[0] <= (long long)g0)) && ((unsigned int)ga1[1] >= (long long)ga1[3]))));
            }
            break;
        default:
            g5 = (short)((unsigned int)(long long)ga0[0] / ((unsigned int)(unsigned long long)g2 | 1u));
        }
    }
    g1 = (unsigned int)((unsigned int)0 - (unsigned int)(long long)g0);
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
    runtime.printf("ga0_0=%llx\n", (unsigned long long)ga0[0]);
    runtime.printf("ga0_1=%llx\n", (unsigned long long)ga0[1]);
    runtime.printf("ga0_2=%llx\n", (unsigned long long)ga0[2]);
    runtime.printf("ga0_3=%llx\n", (unsigned long long)ga0[3]);
    runtime.printf("ga0_4=%llx\n", (unsigned long long)ga0[4]);
    runtime.printf("ga0_5=%llx\n", (unsigned long long)ga0[5]);
    runtime.printf("ga0_6=%llx\n", (unsigned long long)ga0[6]);
    runtime.printf("ga0_7=%llx\n", (unsigned long long)ga0[7]);
    runtime.printf("ga1_0=%llx\n", (unsigned long long)ga1[0]);
    runtime.printf("ga1_1=%llx\n", (unsigned long long)ga1[1]);
    runtime.printf("ga1_2=%llx\n", (unsigned long long)ga1[2]);
    runtime.printf("ga1_3=%llx\n", (unsigned long long)ga1[3]);
    return 0;
}

