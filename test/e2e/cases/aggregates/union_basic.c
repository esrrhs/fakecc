// expect: 42
package main;
union Data {
    int i;
    int j;
};
int main() {
    union Data d;
    d.i = 42;
    /* Union members share storage: d.j reads the same bytes as d.i. */
    return d.j;
}
