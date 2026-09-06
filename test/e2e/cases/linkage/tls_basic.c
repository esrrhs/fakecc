// expect: 42
package main;

__thread int g_tls;

int main() {
    g_tls = 40;
    int *p = &g_tls;
    *p += 2;
    return g_tls;
}
