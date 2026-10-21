#!/usr/bin/env bash
# Differential testing: run every e2e case through both fakecc and an oracle
# compiler (gcc on Linux, clang on macOS), compare exit codes.
set -uo pipefail
# Portability helpers (timeout/nproc/readelf shims for non-GNU hosts).
_COMPAT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
# Abort if the shims could not be sourced: a silently missing `timeout` turns
# every case into a confusing "command not found" instead of a clear error.
. "$_COMPAT_DIR/compat.sh" || { echo "cannot source compat.sh from $_COMPAT_DIR" >&2; exit 2; }
# run_e2e fans cases out to `xargs bash -c` workers, which only see exported
# functions, so re-export the shims this script's workers rely on.
export -f timeout nproc 2>/dev/null || true


ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
FAKECC=${1:-"$ROOT/build/fakecc"}
shift || true
MANIFEST=${MANIFEST:-"$ROOT/test/e2e/difftest_manifest.txt"}
SUITE_DIR="$ROOT/test/e2e/cases"
# Same `<os>-<arch>` tag run_e2e.sh uses for `unsupported_on:` (darwin-arm64).
HOST_TAG="$(host_tag)"
# Oracle compiler: host gcc.  CI installs one on every platform (Homebrew on
# macOS), so both ctest and the workflow legs compare against the same oracle.
# --oracle clang is kept for local experiments only.
ORACLE="gcc"
EXTRA_FLAGS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --oracle)
            ORACLE="$2"
            shift 2
            ;;
        *)
            EXTRA_FLAGS+=("$1")
            shift
            ;;
    esac
done
# Extra fakecc flags (e.g. -O0); the oracle stays unmodified.
FCC_FLAGS="${CC_FLAGS:-} ${EXTRA_FLAGS[*]:-}"

if [ ! -x "$FAKECC" ]; then
    echo "difftest: fakecc not executable: $FAKECC" >&2
    exit 2
fi
# Collect every .c case except the gdb/debug suite (which uses gdb
# breakpoints, not exit codes) and the expect_error cases (rejected
# by design, the oracle would accept them).  Also skip files that import a
# package other than `runtime`: the oracle has no package system, and stripping
# `import flags;` still leaves `flags.CONST` which it cannot parse.
files=()
for f in $(find "$SUITE_DIR" -name '*.c' -not -path '*/debug/*' | sort); do
    if grep -qE '^//[[:space:]]*expect_error' "$f"; then
        continue
    fi
    # Fakecc-runtime behaviour that glibc/libc does not match (locale-dependent
    # %lc/%ls, UTF-8 always, …).  e2e still runs these against expect:.
    if grep -qE '^//[[:space:]]*skip_difftest' "$f"; then
        continue
    fi
    if grep '^import[[:space:]]' "$f" | grep -vqE '^import[[:space:]]+runtime;'; then
        continue
    fi
    # When the oracle is clang, skip tests that use GCC-only builtins or
    # literal forms clang rejects (annotated with `skip_oracle: clang`).
    if [ "$ORACLE" = "clang" ]; then
        if grep -qE '__builtin_va_arg_pack' "$f"; then
            continue
        fi
        if grep -qE '^//[[:space:]]*skip_oracle:[[:space:]]*clang' "$f"; then
            continue
        fi
    fi
    # Oracle gaps that are Darwin's, not fakecc's: Mach-O has no init/fini
    # priorities and only weak aliases, there is no generic syscall(), the SDK
    # headers clash with the libc prototypes the torture ports carry, and
    # Darwin's libc formats %n/%p and scanf differently from glibc (annotated
    # `skip_oracle: darwin`).  e2e still runs these against expect:.
    if [ "$(uname -s)" = "Darwin" ]; then
        if grep -qE '^//[[:space:]]*skip_oracle:[[:space:]]*darwin' "$f"; then
            continue
        fi
    fi
    # Platform-intrinsic skips: run_e2e.sh honours `unsupported_on: <os>-<arch>`
    # (e.g. darwin-arm64) for ABI limits no compiler can satisfy here.  difftest
    # must agree — if e2e will not run the case on this host, comparing it
    # against the oracle is not a signal either.
    skip_host=0
    for t in $(sed -n 's|^//[[:space:]]*unsupported_on:[[:space:]]*\(.*\)|\1|p' "$f" | head -1); do
        if [ "$t" = "$HOST_TAG" ]; then skip_host=1; fi
    done
    if [ "$skip_host" = "1" ]; then
        continue
    fi
    files+=("$f")
done

if [ ${#files[@]} -eq 0 ]; then
    echo "difftest: no cases found in $SUITE_DIR" >&2
    exit 1
fi

exec bash "$ROOT/tools/difftest.sh" -c "$FAKECC" -f "$FCC_FLAGS" -o "$ORACLE" "${files[@]}"
