// expect_error
/* Directory is 'wrongname' but file claims package othername. */
package main;
import wrongname;
int main(void) { return 0; }
