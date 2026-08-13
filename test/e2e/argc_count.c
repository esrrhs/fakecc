// expect: 1
// With no extra argv entries, argc is 1 (the program name).
package main;
int main(int argc, char **argv) {
    (void)argv;
    return argc;
}
