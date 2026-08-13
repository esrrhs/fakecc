// expect: 7
// Call a void function for its side effect (sets a global), then return it.
package main;
int g;
void setg(int v) { g = v; return; }
int main() {
    setg(7);
    return g;
}
