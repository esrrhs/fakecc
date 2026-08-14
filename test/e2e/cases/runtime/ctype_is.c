// ctype is* family: character classification.  Pin the representative
// true/false cases for runtime.isdigit, runtime.isalpha, runtime.isalnum, runtime.isxdigit and runtime.isspace,
// including boundary bytes and a byte well outside each class.
// expect: 0
package main;
import runtime;
int main() {
    /* runtime.isdigit */
    if (!runtime.isdigit('0')) return 1;
    if (!runtime.isdigit('9')) return 2;
    if (runtime.isdigit('/')) return 3;  /* one below '0' */
    if (runtime.isdigit(':')) return 4;  /* one above '9' */
    if (runtime.isdigit('a')) return 5;

    /* runtime.isalpha */
    if (!runtime.isalpha('a')) return 6;
    if (!runtime.isalpha('Z')) return 7;
    if (runtime.isalpha('`')) return 8;  /* one below 'a' */
    if (runtime.isalpha('{')) return 9;  /* one above 'z' */
    if (runtime.isalpha('0')) return 10;

    /* runtime.isalnum: digits or letters */
    if (!runtime.isalnum('5')) return 11;
    if (!runtime.isalnum('m')) return 12;
    if (runtime.isalnum(' ')) return 13;

    /* runtime.isxdigit: 0-9, a-f, A-F */
    if (!runtime.isxdigit('0')) return 14;
    if (!runtime.isxdigit('f')) return 15;
    if (!runtime.isxdigit('F')) return 16;
    if (runtime.isxdigit('g')) return 17;  /* one above 'f' */
    if (runtime.isxdigit('z')) return 18;

    /* runtime.isspace */
    if (!runtime.isspace(' ')) return 19;
    if (!runtime.isspace('\t')) return 20;
    if (!runtime.isspace('\n')) return 21;
    if (runtime.isspace('a')) return 22;

    return 0;
}
