/* This file claims a DIFFERENT package name, so loading dir dup fails the
 * "package name must match directory" check. */
package other;

int g(void) { return 2; }
