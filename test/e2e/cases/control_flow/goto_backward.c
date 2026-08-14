// goto backward: a goto that jumps to an earlier label, the pattern that
// compiles into a back edge in the CFG.  The existing goto tests only
// cover forward jumps.  Here a backward goto drives a loop that sums 1..5.
// expect: 15
package main;
int main() {
    int sum = 0;
    int i = 1;
loop:
    sum = sum + i;
    i = i + 1;
    if (i <= 5) goto loop;
    return sum;
}
