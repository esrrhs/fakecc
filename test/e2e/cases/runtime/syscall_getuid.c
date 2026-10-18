// expect: 0
// Darwin has no generic syscall(); the oracle cannot translate it.
// skip_oracle: darwin
// The named wrapper is mapped only by the arm64 backend (24); x86-64 has the
// raw __syscall(num, ...) form but no name-to-number table for getuid, so
// there is no wrapper to exercise there yet.
// unsupported_on: linux-x86_64
// getuid() exercises the syscall wrapper.  Go through the getuid()
// builtin rather than a raw __syscall number: the call number is 102 on
// Linux/x86-64 but 24 on Darwin arm64, and the backend maps the name to
// the right one per target.  uid is non-negative on normal test setups.
package main;
int main() {
    long uid = __builtin_getuid();
    if (uid < 0) { return 1; }
    return 0;
}
