// expect: 0
// Assigning a scalar to _Complex fills the real part and zeros imag.
package main;
int main(void) {
    _Complex double z;
    z = 3.0;
    if (__real__ z != 3.0) return 1;
    if (__imag__ z != 0.0) return 2;
    _Complex float f;
    f = 1.0f;
    if (__real__ f != 1.0f) return 3;
    if (__imag__ f != 0.0f) return 4;
    return 0;
}
