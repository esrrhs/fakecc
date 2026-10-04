#!/usr/bin/env bash
# Shared POSIX/bash portability helpers for the FakeCC test suites.
#
# The suites are written for a GNU/Linux host and lean on two coreutils that
# macOS does not ship: `timeout` and `nproc`.  Rather than sprinkle `command -v`
# guards through every runner, this file defines shell functions with the same
# names and semantics, and each runner sources it before use.
#
# `timeout` must survive `export -f` + `xargs bash -c`, because run_e2e.sh fans a
# case out to worker shells that only see exported functions.  So it is defined
# with no dependency on non-exportable shell state.
#
# Sourced, never executed.

# Number of parallel jobs to use.  Falls back through the tools macOS does have
# (sysctl) before resorting to a conservative constant.
nproc() {
    # `command -v` would find this very function and recurse, so probe for the
    # real binary with `type -P` (which skips shell functions).
    if type -P nproc >/dev/null 2>&1; then
        nproc
        return
    fi
    local n
    # hw.logicalcpu (hw.physicalcpu is cores, which under-uses the machine).
    n=$(sysctl -n hw.logicalcpu 2>/dev/null || true)
    case "$n" in
        ''|*[!0-9]*) n=4 ;;
    esac
    echo "$n"
}

# Run a command with a wall-clock limit, GNU `timeout` semantics.
#
#   timeout SECONDS CMD [ARG...]
#
# Exit status: CMD's own status when it finishes in time, 124 when the limit is
# hit, and 128+N when CMD is killed by signal N.  Callers in the suites rely on
# 124 to report "timed out" and on 13x to report "killed by a signal", so both
# distinctions are preserved rather than flattened to 124.
#
# GNU timeout also accepts suffixes (30s, 5m); accept them so a caller can pass
# either form.
timeout() {
    local spec="${1:-}"
    [ $# -gt 0 ] && shift

    # Normalize "30" / "30s" / "5m" / "1h" to whole seconds.
    local secs
    case "$spec" in
        ''|*[!0-9smh]*) secs="$spec" ;;
        *s) secs="${spec%s}" ;;
        *m) secs=$(( ${spec%m} * 60 )) ;;
        *h) secs=$(( ${spec%h} * 3600 )) ;;
        *)  secs="$spec" ;;
    esac
    case "$secs" in
        ''|*[!0-9]*)
            echo "timeout: invalid time interval '$spec'" >&2
            return 2
            ;;
    esac

    "$@" &
    local pid=$!

    # Poll for completion.  `wait` cannot be interrupted portably in bash 3.2
    # (the version macOS ships), so poll `kill -0` and then reap.
    local waited=0
    while kill -0 "$pid" 2>/dev/null; do
        if [ "$waited" -ge "$secs" ]; then
            kill -TERM "$pid" 2>/dev/null || true
            # Give it a moment to die politely, then insist.
            local grace=0
            while kill -0 "$pid" 2>/dev/null && [ "$grace" -lt 5 ]; do
                sleep 1
                grace=$(( grace + 1 ))
                kill -TERM "$pid" 2>/dev/null || true
            done
            kill -KILL "$pid" 2>/dev/null || true
            wait "$pid" 2>/dev/null
            return 124
        fi
        sleep 1
        waited=$(( waited + 1 ))
    done

    # Finished on its own: propagate the real status, preserving 128+N.
    wait "$pid"
    return $?
}

# Read ELF dynamic tags.  Linux ships readelf; macOS does not, so use otool and
# dyld-oriented spellings instead.  Prints the same shape of lines the callers
# grep for, i.e. lines that contain a tag name and its value.
#
#   elf_dynamic_dump BINARY
elf_dynamic_dump() {
    local bin="$1"
    if command -v readelf >/dev/null 2>&1; then
        LANG=C readelf -d "$bin" 2>/dev/null
        return
    fi
    # Mach-O fallback: map the two tags the suites actually query onto otool's
    # install-name/weak-reference vocabulary.
    local install
    install=$(otool -l "$bin" 2>/dev/null) || return 0
    printf '%s\n' "$install" | awk '
        /^ *cmd LC_LOAD_DYLIB/ { want = 1; next }
        want && /^ *name / {
            line = $0
            sub(/^ *name /, "", line)
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", line)
            printf "(NEEDED) %s\n", line
            want = 0
        }
    '
}

# True when the dynamic section of BINARY mentions SONAME.
has_needed() {
    local bin="$1" soname="$2"
    elf_dynamic_dump "$bin" | grep '(NEEDED)' | grep -F "[$soname]" >/dev/null
}

# True when the dynamic section of BINARY mentions a runpath containing NEEDLE.
has_runpath() {
    local bin="$1" needle="$2"
    elf_dynamic_dump "$bin" | grep '(RUNPATH)' | grep -F "$needle" >/dev/null
}