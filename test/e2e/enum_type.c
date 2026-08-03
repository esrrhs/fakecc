// expect: 1
package main;
enum Color { RED, GREEN, BLUE };
int main() {
    /* Enum used as a variable type — lowered to int. */
    enum Color c;
    c = GREEN;   /* 1 */
    return c;
}
