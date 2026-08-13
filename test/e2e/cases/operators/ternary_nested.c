// expect: 2
package main;
int main() {
    /* Right-associative: a ? b : c ? d : e  ===  a ? b : (c ? d : e) */
    int a = 0;
    int b = 9;
    int c = 1;
    int d = 2;
    int e = 3;
    return (a ? b : c ? d : e);
}
