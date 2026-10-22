/* char and short loads, integer promotions, mixed signedness. */
package main;
import runtime;

enum { N = 200000 };
enum { ROUNDS = 40 };

static unsigned char bytes[N];
static signed char sbytes[N];
static unsigned short words[N];
static short swords[N];

int main(void) {
    int i, r;
    unsigned sum;

    for (i = 0; i < N; i++) {
        bytes[i] = (unsigned char)(i * 3);
        sbytes[i] = (signed char)(i * 5);
        words[i] = (unsigned short)(i * 13);
        swords[i] = (short)(i * 9);
    }
    for (r = 0; r < ROUNDS; r++) {
        for (i = 0; i < N; i++) {
            int t = (int)sbytes[i] + (int)swords[i];
            unsigned u = (unsigned)bytes[i] + (unsigned)words[i];
            bytes[i] = (unsigned char)(u + (unsigned)t + (unsigned)r);
            swords[i] = (short)(t ^ (int)words[i]);
        }
    }
    sum = 0;
    for (i = 0; i < N; i += 8)
        sum += bytes[i] + (unsigned)(unsigned short)swords[i];
    runtime.printf("narrow %u\n", sum);
    return 0;
}
