// expect: 1
package main;
int main() {
    /* C precedence (low to high): | < ^ < & < == < < < << < + < *
     * 1 | 2 & 4  ==  1 | (2 & 4)  == 1 | 0  == 1  (& binds tighter than |) */
    return (1 | 2 & 4);
}
