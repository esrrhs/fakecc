#!/usr/bin/env bash
# Install-layout smoke test.
#
#   install_smoke.sh BUILD_DIR
#
# Everything else in the suite drives fakecc out of the build tree, where the
# runtime package is picked up from ./runtime next to the sources.  An
# *installed* compiler reaches it by a completely different branch of the
# search -- ../share/fakecc/runtime relative to argv[0] -- so nothing else here
# exercises that path.  Which means a broken install rule is invisible: CI
# stays green and the installed compiler simply cannot find its runtime.
#
# So install into a scratch prefix and then actually use the result, from a
# working directory that has nothing to do with the repository.
set -uo pipefail

BUILD=${1:-build}

if [ ! -d "$BUILD" ]; then
    echo "FAIL install_smoke: build dir '$BUILD' does not exist" >&2
    exit 1
fi

PREFIX=$(mktemp -d)
WORK=$(mktemp -d)
trap 'rm -rf "$PREFIX" "$WORK"' EXIT

fail() { echo "FAIL install_smoke: $*"; exit 1; }
ok()   { echo "PASS install_smoke: $*"; }

cmake --install "$BUILD" --prefix "$PREFIX" >/dev/null 2>&1 ||
    fail "cmake --install $BUILD --prefix \$TMP failed"

# 1. The compiler itself has to land on the standard bindir.
# Windows installs fakecc.exe; the other hosts install fakecc.
FCC="$PREFIX/bin/fakecc"
if [ ! -x "$FCC" ] && [ -x "$FCC.exe" ]; then
    FCC="$FCC.exe"
fi
[ -x "$FCC" ] || fail "no executable at \$PREFIX/bin/fakecc"

# 2. The runtime package.  This is the rule that can quietly go missing:
#    runtime/ has its own install(DIRECTORY ...), and a compiler without it
#    dies on the first program that prints anything.  Match on the path shape
#    rather than a hard-coded `share`, because CMAKE_INSTALL_DATADIR is not
#    the same everywhere.
n=$(find "$PREFIX" -type f -name '*.c' -path '*fakecc/runtime/*' 2>/dev/null | wc -l)
[ "$n" -gt 0 ] || fail "no runtime sources installed under \$PREFIX/*/fakecc/runtime"
ok "layout: bin/fakecc + $n runtime sources"

# 3. Compile and run with it, from an unrelated directory.  Using runtime
#    at all is the point: a compiler that found no runtime package fails here
#    even though it links a bare `return 0;` fine.
cat >"$WORK/hello.c" <<'EOF'
package main;

import runtime;

int main() {
    runtime.printf("installed\n");
    return 7;
}
EOF

(cd "$WORK" && "$FCC" hello.c -o hello) >/dev/null 2>&1 ||
    fail "installed fakecc could not compile hello.c"
[ -x "$WORK/hello" ] || fail "installed fakecc produced no executable"

out=$(cd "$WORK" && ./hello 2>/dev/null)
rc=$?
[ "$rc" = 7 ] || fail "installed binary exited $rc, expected 7"
case "$out" in
    *installed*) ;;
    *) fail "installed binary printed '$out', expected 'installed'" ;;
esac

ok "installed fakecc compiles and runs a runtime program (exit 7, stdout '$out')"
