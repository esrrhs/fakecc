// expect: 0
// Complex multiply: (2+3i)*(4+5i) = -7+22i.  Integer, float, and double
// paths must all produce that result.
package main;

int main(void) {
    _Complex int zi = 2 + 3i;
    _Complex int wi = 4 + 5i;
    _Complex int pi = zi * wi;
    if (__real__ pi != -7) return 1;
    if (__imag__ pi != 22) return 2;

    _Complex float zf = 2.0f + 3.0fi;
    _Complex float wf = 4.0f + 5.0fi;
    _Complex float pf = zf * wf;
    if (__real__ pf != -7.0f) return 3;
    if (__imag__ pf != 22.0f) return 4;

    _Complex double zd = 2.0 + 3.0i;
    _Complex double wd = 4.0 + 5.0i;
    _Complex double pd = zd * wd;
    if (__real__ pd != -7.0) return 5;
    if (__imag__ pd != 22.0) return 6;

    return 0;
}
