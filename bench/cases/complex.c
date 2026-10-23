/* _Complex float/double add and multiply in a hot loop. */
package main;
import runtime;

enum { N = 6000000 };

int main(void) {
    unsigned i;
    _Complex float zf, wf;
    _Complex double zd, wd;
    int sr, si;
    unsigned acc;

    zf = 1.0f + 0.5fi;
    wf = 0.25f + 0.125fi;
    zd = 1.0 + 0.5i;
    wd = 0.25 + 0.125i;
    acc = 0;
    for (i = 0; i < N; i++) {
        zf = zf * wf + (float)(i & 3u);
        wf = zf + wf;
        /* Keep magnitude from exploding: fold back toward the unit disk. */
        zf = zf * 0.5f;
        wf = wf * 0.5f;

        zd = zd * wd + (double)(i & 3u);
        wd = zd + wd;
        zd = zd * 0.5;
        wd = wd * 0.5;

        sr = (int)(__real__ zf * 256.0f) + (int)(__real__ zd * 256.0);
        si = (int)(__imag__ zf * 256.0f) + (int)(__imag__ zd * 256.0);
        acc += (unsigned)(sr ^ (si << 1));
    }
    runtime.printf("complex %u %d %d\n", acc, sr, si);
    return 0;
}
