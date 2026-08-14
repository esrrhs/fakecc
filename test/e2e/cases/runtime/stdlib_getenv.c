// std.getenv: looks up an environment variable by reading
// /proc/self/environ.  Pin that a known variable (PATH) is found and
// non-empty, and that a definitely-absent name returns NULL.
// expect: 0
package main;
import std;
int main() {
    char *p = std.getenv("PATH");
    if (p == 0) return 1;       /* PATH is essentially always set */
    if (p[0] == 0) return 2;    /* and carries a non-empty value */

    /* a name that cannot exist */
    char *q = std.getenv("fakecc_env_var_absent_xyz");
    if (q != 0) return 3;

    return 0;
}
