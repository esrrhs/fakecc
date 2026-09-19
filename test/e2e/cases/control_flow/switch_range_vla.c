// expect: 0
// GNU case ranges, a VLA in the taken arm, and a computed goto.
package main;
int main(void) {
    int n = 3;
    switch (n) {
    case 1 ... 5: {
        int a[n];
        a[0] = 7;
        if (a[0] != 7) return 1;
        break;
    }
    default:
        return 2;
    }
    void *p = &&done;
    switch (1) {
    case 1:
        goto *p;
    default:
        return 3;
    }
    return 4;
done:
    return 0;
}
