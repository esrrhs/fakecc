// ctype is* family: character classification.  Pin the representative
// true/false cases for isdigit, isalpha, isalnum, isxdigit and isspace,
// including boundary bytes and a byte well outside each class.
// expect: 0
package main;
extern int isdigit(int c);
extern int isalpha(int c);
extern int isalnum(int c);
extern int isxdigit(int c);
extern int isspace(int c);
int main() {
    /* isdigit */
    if (!isdigit('0')) return 1;
    if (!isdigit('9')) return 2;
    if (isdigit('/')) return 3;  /* one below '0' */
    if (isdigit(':')) return 4;  /* one above '9' */
    if (isdigit('a')) return 5;

    /* isalpha */
    if (!isalpha('a')) return 6;
    if (!isalpha('Z')) return 7;
    if (isalpha('`')) return 8;  /* one below 'a' */
    if (isalpha('{')) return 9;  /* one above 'z' */
    if (isalpha('0')) return 10;

    /* isalnum: digits or letters */
    if (!isalnum('5')) return 11;
    if (!isalnum('m')) return 12;
    if (isalnum(' ')) return 13;

    /* isxdigit: 0-9, a-f, A-F */
    if (!isxdigit('0')) return 14;
    if (!isxdigit('f')) return 15;
    if (!isxdigit('F')) return 16;
    if (isxdigit('g')) return 17;  /* one above 'f' */
    if (isxdigit('z')) return 18;

    /* isspace */
    if (!isspace(' ')) return 19;
    if (!isspace('\t')) return 20;
    if (!isspace('\n')) return 21;
    if (isspace('a')) return 22;

    return 0;
}
