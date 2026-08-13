// expect: 8
package main;
union Mix {
    long big;
    int small;
};
int main() {
    /* Union size = max member size = sizeof(long) = 8. */
    return sizeof(union Mix);
}
