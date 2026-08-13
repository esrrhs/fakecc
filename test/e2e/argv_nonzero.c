// expect: 0
// argv is non-NULL and argv[0] points at a non-empty program name.
package main;
int main(int argc, char **argv) {
    if (argc < 1) return 1;
    if (argv == 0) return 2;
    if (argv[0] == 0) return 3;
    if (argv[0][0] == 0) return 4;
    return 0;
}
