// expect: 3
// Self-referential struct via pointer, using -> operator.
package main;
struct Node { int val; struct Node *next; };
int main() {
    struct Node a;
    struct Node b;
    a.val = 1;
    a.next = &b;
    b.val = 2;
    b.next = 0;
    return a.val + a.next->val;
}
