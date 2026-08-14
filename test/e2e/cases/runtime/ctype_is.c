// ctype is* family: character classification.  Pin the representative
// true/false cases for ctype.isdigit, ctype.isalpha, ctype.isalnum, ctype.isxdigit and ctype.isspace,
// including boundary bytes and a byte well outside each class.
// expect: 0
package main;
import ctype;
int main() {
    /* ctype.isdigit */
    if (!ctype.isdigit('0')) return 1;
    if (!ctype.isdigit('9')) return 2;
    if (ctype.isdigit('/')) return 3;  /* one below '0' */
    if (ctype.isdigit(':')) return 4;  /* one above '9' */
    if (ctype.isdigit('a')) return 5;

    /* ctype.isalpha */
    if (!ctype.isalpha('a')) return 6;
    if (!ctype.isalpha('Z')) return 7;
    if (ctype.isalpha('`')) return 8;  /* one below 'a' */
    if (ctype.isalpha('{')) return 9;  /* one above 'z' */
    if (ctype.isalpha('0')) return 10;

    /* ctype.isalnum: digits or letters */
    if (!ctype.isalnum('5')) return 11;
    if (!ctype.isalnum('m')) return 12;
    if (ctype.isalnum(' ')) return 13;

    /* ctype.isxdigit: 0-9, a-f, A-F */
    if (!ctype.isxdigit('0')) return 14;
    if (!ctype.isxdigit('f')) return 15;
    if (!ctype.isxdigit('F')) return 16;
    if (ctype.isxdigit('g')) return 17;  /* one above 'f' */
    if (ctype.isxdigit('z')) return 18;

    /* ctype.isspace */
    if (!ctype.isspace(' ')) return 19;
    if (!ctype.isspace('\t')) return 20;
    if (!ctype.isspace('\n')) return 21;
    if (ctype.isspace('a')) return 22;

    return 0;
}
