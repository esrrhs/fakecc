/* A tiny user package exercising cross-package function import.  Both
 * functions are non-static so they are exported; the directory name must
 * match the package declaration for import calc to resolve. */
package calc;

int add(int a, int b) { return a + b; }

int mul(int a, int b) { return a * b; }
