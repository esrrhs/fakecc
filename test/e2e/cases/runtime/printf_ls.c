// expect: 0
package main;
import runtime;
int main(void) {
    int hi[3];
    hi[0] = 104; hi[1] = 105; hi[2] = 0;
    int *p = hi;
    char buf[8];
    buf[0] = 'Z'; buf[1] = 'Z'; buf[2] = 'Z';
    int n = runtime.sprintf(buf, "%ls", p);
    if (n != 2) return 10+n;
    if (buf[0]=='h' && buf[1]=='i' && buf[2]==0) return 0;
    if (buf[0]=='Z') return 1;
    return (int)(unsigned char)buf[0];
}
