// expect_error
/* static symbols are package-private and must not be reachable via import. */
package main;
import hide;
int main(void) {
    return hide.secret();
}
