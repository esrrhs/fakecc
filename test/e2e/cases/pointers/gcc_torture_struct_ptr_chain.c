// expect: 0
// Ported from GCC C-Torture suite: gcc.c-torture/execute/930603-1.c
package main;

struct node {
    int val;
    struct node *next;
};

int main() {
    struct node c = {30, 0};
    struct node b = {20, &c};
    struct node a = {10, &b};

    int sum = 0;
    for (struct node *p = &a; p; p = p->next) {
        sum += p->val;
    }

    if (sum != 60) return 1;
    return 0;
}
