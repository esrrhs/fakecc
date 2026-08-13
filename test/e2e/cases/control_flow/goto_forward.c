// expect: 99
package main;
int main() {
    int x = 0;
    goto done;
    return x;
done:
    x = 99;
    return x;
}
