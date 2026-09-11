// expect: 0
// `extern int x; int x;` — the tentative definition gives the object BSS
// storage.  Reading it must not fail at link time.
package main;
extern int x;
int x;
int main(void) { return x; }
