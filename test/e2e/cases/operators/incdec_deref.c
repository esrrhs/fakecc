// expect: 6
package main;
int main() {
    /* a[0]++ exercises the general (address-taken) path: the array element
     * is pinned, so the load/add/store goes through its address. Postfix
     * returns the old value (5), then a[0] becomes 6. */
    int a[2];
    a[0] = 5;
    a[1] = 9;
    int r = a[0]++;       /* r = 5, a[0] becomes 6 */
    return r + a[0] - 5;  /* 5 + 6 - 5 = 6 */
}
