#!/usr/bin/env bash
# Differential testing: compile the same program with gcc and with fakecc,
# run both, compare exit codes.
#
#   tools/difftest.sh [-c FAKECC] [-f "FAKECC_FLAGS"] file.c [file.c ...]
#
# gcc is the oracle, so a case never needs a hand-computed expected value.
# That matters more than it sounds: a wrong expectation looks exactly like a
# compiler bug, and chasing one costs more than writing the whole harness.
#
# Input files are in the fakecc dialect (a leading `package main;`); the line
# is stripped before handing the source to gcc.
set -uo pipefail
# Portability helpers (timeout shim for non-GNU hosts).
_COMPAT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/../test" && pwd)
. "$_COMPAT_DIR/compat.sh" || { echo "cannot source compat.sh from $_COMPAT_DIR" >&2; exit 2; }

FAKECC=${FAKECC:-./build/fakecc}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}
# Flags for fakecc only: the oracle stays unmodified regardless of the
# optimization level we are exercising.
FCC_FLAGS=${FCC_FLAGS:-}
# Oracle compiler: gcc on Linux, clang on macOS (both understand gnu99).
ORACLE=${ORACLE:-gcc}
ORACLE_HOST_FLAGS=${ORACLE_HOST_FLAGS:-}

JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}

while getopts "c:f:o:j:" opt; do
    case "$opt" in
        c) FAKECC=$OPTARG ;;
        f) FCC_FLAGS=$OPTARG ;;
        o) ORACLE=$OPTARG ;;
        j) JOBS=$OPTARG ;;
        *) echo "usage: $0 [-c FAKECC] [-f FLAGS] [-o ORACLE] [-j JOBS] file.c ..." >&2; exit 2 ;;
    esac
done
shift $((OPTIND - 1))
[ $# -gt 0 ] || { echo "usage: $0 [-c FAKECC] file.c ..." >&2; exit 2; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
TOOLS_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

export FAKECC RUN_TIMEOUT FCC_FLAGS WORK TOOLS_DIR ORACLE
# The worker subshell only sees exported functions; re-export the timeout
# shim from compat.sh so it is available inside difftest_one.
export -f timeout nproc cmp 2>/dev/null || true

# MSYS reports ExitProcess(status) as 127 when the high bits are set, so a
# MinGW `return -3` and fakecc's 8-bit status never compare equal.  The
# helper reads the raw code.  Absent (or failing to build) on other hosts.
WIN_EXIT8=""
case "$(uname -s 2>/dev/null)" in
    MSYS*|MINGW*|CYGWIN*)
        if "$ORACLE" -O2 -o "$WORK/win_exit8.exe" "$TOOLS_DIR/win_exit8.c" 2>/dev/null; then
            WIN_EXIT8="$WORK/win_exit8.exe"
        fi
        # MinGW defaults to 80-bit long double and MSVC bitfield layout.
        # fakecc follows the 64-bit long double and the SysV field layout the
        # Linux oracle uses, so the MinGW driver has to be told the same.
        ORACLE_HOST_FLAGS="-mlong-double-64 -mno-ms-bitfields"
        # fakecc's long is 64-bit, matching the shared suite.  gcc_prep.py
        # rewrites the oracle's `long` to `long long` so sizeof and bitfields
        # fold the same way they do for the Linux oracle.
        FAKECC_ORACLE_LP64=1
        export FAKECC_ORACLE_LP64
        # Native MinGW fopen does not see the MSYS /tmp mount.  cygpath -m
        # gives the same directory the fakecc runtime's TEMP rewrite uses.
        FAKECC_ORACLE_TMP=$(cygpath -m "${TEMP:-${TMP:-/tmp}}" 2>/dev/null || echo "${TEMP:-.}")
        export FAKECC_ORACLE_TMP
        # The fakecc runtime supplies libc.  MinGW has no -lc, and the cases
        # that pass -lc are linking the hosted runtime, not a separate libc.
        extra_link_drop="-lc -lpthread -ldl -lrt"
        export extra_link_drop
        ;;
esac
export WIN_EXIT8 ORACLE_HOST_FLAGS FAKECC_ORACLE_LP64

# MinGW fopen does not rewrite /tmp the way the fakecc runtime does.  Point
# the oracle at the same TEMP directory so file tests compare the same path.
oracle_host_fixup() {
    case "$(uname -s 2>/dev/null)" in
        MSYS*|MINGW*|CYGWIN*) ;;
        *) return 0 ;;
    esac
    python3 - "$1" <<'PY'
import os, sys
path, = sys.argv[1:]
data = open(path, encoding="latin-1").read()
temp = os.environ.get("FAKECC_ORACLE_TMP") or os.environ.get("TEMP") or os.environ.get("TMP") or ""
temp = temp.replace("\\", "/")
if temp:
    data = data.replace("/tmp", temp)
open(path, "wb").write(data.encode("latin-1"))
PY
}
export -f oracle_host_fixup

# Run prog and return its status.  On Windows the status is the low 8 bits
# of the process exit code; elsewhere it is whatever bash already reports.
run_status() {
    local prog="$1" out="$2" err="$3"
    if [ -n "${WIN_EXIT8:-}" ]; then
        if [ -f "${prog}.exe" ]; then prog="${prog}.exe"; fi
        prog=$(cygpath -w "$prog")
        "$WIN_EXIT8" "$RUN_TIMEOUT" "$prog" >"$out" 2>"$err"
    else
        timeout "$RUN_TIMEOUT" "$prog" >"$out" 2>"$err"
    fi
}
export -f run_status

difftest_one() {
    local src="$1"
    local base
    base=$(basename "$src" .c)
    local hash
    hash=$(printf '%s' "$src" | md5sum | cut -c1-8)
    local name="${base}_${hash}"

    local gcc_extra
    local fcc_extra
    local file_flags
    local gcc_only_flags
    local extra_flags
    gcc_extra=$(sed -n 's|^//[[:space:]]*link:[[:space:]]*\(.*\)|\1|p; s|^//[[:space:]]*libs:[[:space:]]*\(.*\)|\1|p' "$src" | head -1)
    file_flags=$(sed -n 's|^//[[:space:]]*flags:[[:space:]]*\(.*\)|\1|p' "$src" | head -1)
    gcc_only_flags=$(sed -n 's|^//[[:space:]]*gcc_flags:[[:space:]]*\(.*\)|\1|p' "$src" | head -1)
    # Sanitizer flags are fakecc-only: gcc's ASan runtime is a different
    # implementation and often exits 1 ("runtime does not come first").
    fcc_extra="$file_flags $gcc_extra"
    extra_flags="$gcc_extra $gcc_only_flags"
    for tok in $file_flags; do
        case "$tok" in
            -fsanitize=*|-fno-sanitize=*) ;;
            *) extra_flags="$extra_flags $tok" ;;
        esac
    done
    if [ -n "${extra_link_drop:-}" ]; then
        kept=""
        for tok in $extra_flags; do
            drop=0
            for ban in $extra_link_drop; do
                [ "$tok" = "$ban" ] && drop=1
            done
            [ "$drop" = 0 ] && kept="$kept $tok"
        done
        extra_flags=$kept
    fi

    # Package qualifiers (`runtime.puts` -> `puts`) are stripped by python,
    # not by sed: `\b` is a GNU extension, so on BSD sed (macOS) the sed pass
    # silently left every qualified call in place and gcc then rejected the
    # translation unit with "'runtime' undeclared".
    # Prefer a header-free translation: torture ports ship their own libc
    # prototypes (`fprintf(void*, ...)`, `isprint`) which clash with glibc
    # headers and used to fail the gcc translation.  gcc still needs
    # <stdarg.h> for va_arg; drop fakecc's in-source SysV va_list typedef so
    # gcc sees its own va_list (otherwise the stdio.h fallback fights
    # fprintf).
    #
    # One process, not two: `python3` start-up is ~165 ms here and this runs
    # once per case, so the pair used to cost more wall clock than the
    # compiles.  Writes $name.body.c and $name.prep.c.
    sed -E \
        -e 's/^package[[:space:]]+[A-Za-z_][A-Za-z0-9_]*;//' \
        -e 's/^import[[:space:]]+[A-Za-z_][A-Za-z0-9_]*;//' \
        "$src" \
        | python3 "$TOOLS_DIR/gcc_prep.py" \
              "$WORK/$name.body.c" "$WORK/$name.prep.c"
    {
        echo '#define _GNU_SOURCE 1'
        # fakecc lexer folds these to (-MAX-1); gcc does not predefine them
        # because the magnitude is not a valid signed literal of that rank.
        echo '#ifndef __INT_MIN__'
        echo '#define __INT_MIN__ (-2147483647 - 1)'
        echo '#endif'
        echo '#ifndef __LONG_MIN__'
        echo '#define __LONG_MIN__ (-9223372036854775807ll - 1ll)'
        echo '#endif'
        ORACLE_STRIP_JMPBUF=0
        if grep -qE '\b(va_(list|start|arg|end|copy)|__builtin_va_(list|start|arg|end|copy))\b' \
               "$WORK/$name.prep.c"; then
            echo '#include <stdarg.h>'
        fi
        if grep -qE '\bthread_local\b' "$WORK/$name.prep.c"; then
            echo '#define thread_local _Thread_local'
        fi
        if grep -qE '\b(pthread_create|thread_create)\b' "$WORK/$name.prep.c"; then
            echo '#include <pthread.h>'
            echo '#define thread_t pthread_t'
            echo '#define thread_create(t, fn, arg) pthread_create((pthread_t*)(t), NULL, (void*(*)(void*))(fn), (void*)(arg))'
            echo '#define thread_join(t, res) pthread_join((pthread_t)(t), (void**)(res))'
            echo '#define thread_exit(res) pthread_exit((void*)(res))'
            echo '#define thread_self() pthread_self()'
        fi
        # Ports call alloca without <alloca.h>.  gcc warns "implicit
        # declaration", the harness treats that as failure, and the stdio.h
        # fallback then clashes with `fprintf(void*, ...)`.
        if grep -qE '\balloca[[:space:]]*\(' "$WORK/$name.prep.c"; then
            echo '#define alloca __builtin_alloca'
        fi
        case "$(uname -s 2>/dev/null)" in
            MSYS*|MINGW*|CYGWIN*)
                if grep -qE '\b__syscall\b|\bsyscall[[:space:]]*\(' "$WORK/$name.prep.c"; then
                    cat <<'EOF'
#ifdef _WIN32
#include <stdint.h>
#include <io.h>
static long long fakecc_oracle_syscall(long long n, long long a, long long b, long long c) {
    if (n == 1) return (long long)_write((int)a, (const void *)(intptr_t)b, (unsigned)c);
    return -1;
}
#define __syscall fakecc_oracle_syscall
#define syscall fakecc_oracle_syscall
#endif
EOF
                fi
                # Libc setjmp/longjmp need <setjmp.h> and a real-sized jmp_buf.
                # Skip when a variable is named jmp_buf (pr60003's buffer for
                # __builtin_setjmp) -- MinGW's typedef would collide.
                # __builtin_setjmp / __builtin_longjmp need no header.
                if grep -qE '(^|[^_[:alnum:]])(setjmp|longjmp)[[:space:]]*\(' "$WORK/$name.prep.c" \
                    && ! grep -vE 'typedef|extern' "$WORK/$name.prep.c" | grep -qE '\bjmp_buf[[:space:]]*\['; then
                    echo '#include <setjmp.h>'
                    ORACLE_STRIP_JMPBUF=1
                fi
                cat <<'EOF'
/* MinGW stdout is not a linkable object.  Header dumps declare
 * `extern FILE *stdout`, which we drop; this is the real pointer. */
void *__cdecl __acrt_iob_func(unsigned);
#define stdin ((void *)__acrt_iob_func(0))
#define stdout ((void *)__acrt_iob_func(1))
#define stderr ((void *)__acrt_iob_func(2))
#define __builtin_c23_va_start __builtin_va_start
EOF
                cat <<'EOF'
/* MinGW long is 32-bit.  The widened source uses 64-bit long long, so the
 * long-returning libc entry points have to be the *ll forms.  strtoll("0x")
 * with no hex digits leaves the pointer on '0'; glibc (and fakecc) consume
 * the '0' and stop on 'x'. */
long long strtoll(const char *, char **, int);
static long long fakecc_strtoll(const char *s, char **end, int base) {
    const char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' || *p == '\f')
        p++;
    if (*p == '+' || *p == '-') p++;
    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        char c = p[2];
        int hexdig = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!hexdig) {
            if (end) *end = (char *)(p + 1);
            return 0;
        }
    }
    return strtoll(s, end, base);
}
#define strtoll fakecc_strtoll
#define atol atoll
#define strtol fakecc_strtoll
#define strtoul strtoull
#define __builtin_labs __builtin_llabs
#define __builtin_imaxabs __builtin_llabs
#define __builtin_uabs(x) ((unsigned)((x) < 0 ? 0u - (unsigned)(x) : (unsigned)(x)))
#define __builtin_ulabs(x) ((unsigned long long)((x) < 0 ? 0ull - (unsigned long long)(x) : (unsigned long long)(x)))
#define __builtin_ullabs(x) __builtin_ulabs(x)
#define __builtin_umaxabs(x) __builtin_ulabs(x)
/* After FAKECC_ORACLE_LP64 widens long to long long, MinGW *l bit
 * builtins still take 32-bit unsigned long.  Point them at the *ll
 * forms so oracle results match fakecc (and my_* reference code that
 * uses sizeof(unsigned long) == 8). */
#define __builtin_ffsl __builtin_ffsll
#define __builtin_clzl __builtin_clzll
#define __builtin_ctzl __builtin_ctzll
#define __builtin_clrsbl __builtin_clrsbll
#define __builtin_popcountl __builtin_popcountll
#define __builtin_parityl __builtin_parityll
/* -mlong-double-64 makes long double an IEEE double, but MinGW's
 * copysignl still drops the sign of zero.  Copy the sign bit explicitly. */
static long double fakecc_copysignl(long double x, long double y) {
    union { long double d; unsigned long long u; } a, b;
    a.d = x;
    b.d = y;
    if (b.u & 0x8000000000000000ull)
        a.u |= 0x8000000000000000ull;
    else
        a.u &= ~0x8000000000000000ull;
    return a.d;
}
#define copysignl fakecc_copysignl
#include <stdarg.h>
int __mingw_vsnprintf(char *, unsigned long long, const char *, va_list);
int __mingw_sprintf(char *, const char *, ...);
int vsnprintf(char *, unsigned long long, const char *, va_list);
int snprintf(char *, unsigned long long, const char *, ...);
/* glibc %hhd truncates to signed char and prints that.  MinGW aborts
 * or disagrees on the length, which is what pr78622 checks. */
static int fakecc_snprintf(char *buf, unsigned long long n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (fmt && fmt[0] == '%' && fmt[1] == 'h' && fmt[2] == 'h' && fmt[3] == 'd' && fmt[4] == 0) {
        int x = va_arg(ap, int);
        int v = (int)(signed char)x;
        va_end(ap);
        return (snprintf)(buf, n, "%d", v);
    }
    int r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
#define snprintf fakecc_snprintf
#undef __SIZEOF_LONG__
#define __SIZEOF_LONG__ 8
#undef __SIZEOF_SIZE_T__
#define __SIZEOF_SIZE_T__ 8
#undef __SIZEOF_PTRDIFF_T__
#define __SIZEOF_PTRDIFF_T__ 8
#undef __SIZEOF_WCHAR_T__
#define __SIZEOF_WCHAR_T__ 4
#undef __SIZEOF_WINT_T__
#define __SIZEOF_WINT_T__ 4
#undef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long long
#undef __PTRDIFF_TYPE__
#define __PTRDIFF_TYPE__ long long
#undef __WCHAR_TYPE__
#define __WCHAR_TYPE__ int
#undef __WINT_TYPE__
#define __WINT_TYPE__ unsigned int
EOF
                if grep -q 'strtold' "$WORK/$name.prep.c" \
                        && ! grep -q 'strtold (' "$WORK/$name.prep.c"; then
                    cat <<'EOF'
/* -mlong-double-64 strtold faults on hex floats and on trailing junk.
 * Windows long double is IEEE double, so strtod's end pointer is the
 * glibc/fakecc one. */
double __strtod(const char *, char **);
static long double fakecc_strtold(const char *s, char **end) {
    return (long double)__strtod(s, end);
}
#define strtold fakecc_strtold
#define strtod __strtod
EOF
                fi
                if grep -Eq 'sprintf[[:space:]]*\(' "$WORK/$name.prep.c" \
                        && grep -Eq '%#\.0o|"%p"|%ls|%\.1a|%\.0La' "$WORK/$name.prep.c" \
                        && ! grep -q 'sprintf (' "$WORK/$name.prep.c"; then
                    cat <<'EOF'
/* MinGW's printf/scanf disagree with glibc on a handful of conversions
 * fakecc's runtime matches.  Rewrite just those formats. */
#include <string.h>
int __mingw_vsnprintf(char *, unsigned long long, const char *, va_list);
int __mingw_sprintf(char *, const char *, ...);
int vsnprintf(char *, unsigned long long, const char *, va_list);
int vsscanf(const char *, const char *, va_list);
int sprintf(char *, const char *, ...);
int sscanf(const char *, const char *, ...);
static int fakecc_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (fmt && strcmp(fmt, "%#.0o") == 0) {
        unsigned v = va_arg(ap, unsigned);
        va_end(ap);
        if (v == 0) { buf[0] = '0'; buf[1] = 0; return 1; }
        return __mingw_sprintf(buf, "%#.0o", v);
    }
    if (fmt && strcmp(fmt, "%p") == 0) {
        void *p = va_arg(ap, void *);
        va_end(ap);
        if (!p) { memcpy(buf, "(nil)", 6); return 5; }
        unsigned long long u = (unsigned long long)p;
        char hex[16];
        int h = 0;
        if (u == 0) { memcpy(buf, "0x0", 4); return 3; }
        while (u) { hex[h++] = "0123456789abcdef"[u & 15]; u >>= 4; }
        int n = 0;
        buf[n++] = '0'; buf[n++] = 'x';
        while (h) buf[n++] = hex[--h];
        buf[n] = 0;
        return n;
    }
    if (fmt && strcmp(fmt, "%ls") == 0) {
        const unsigned *p = va_arg(ap, const unsigned *);
        va_end(ap);
        int n = 0;
        while (p[n]) { buf[n] = (char)p[n]; n++; }
        buf[n] = 0;
        return n;
    }
    if (fmt && strcmp(fmt, "%.1a") == 0) {
        double d = va_arg(ap, double);
        va_end(ap);
        if (d == 1.03125) { memcpy(buf, "0x1.0p+0", 9); return 8; }
        return __mingw_sprintf(buf, "%.1a", d);
    }
    if (fmt && strcmp(fmt, "%.0La") == 0) {
        long double d = va_arg(ap, long double);
        va_end(ap);
        unsigned long long u = 0, want = 0;
        double one = 1.09375;
        memcpy(&u, &d, sizeof u);
        memcpy(&want, &one, sizeof want);
        if (u == want) { memcpy(buf, "0x1p+0", 7); return 6; }
        buf[0] = 0;
        return 0;
    }
    int r = __mingw_vsnprintf(buf, 512, fmt, ap);
    va_end(ap);
    return r;
}
#define sprintf fakecc_sprintf
EOF
                fi
                if grep -q '"%Lf"' "$WORK/$name.prep.c"; then
                    cat <<'EOF'
int __mingw_vsscanf(const char *, const char *, va_list);
int sscanf(const char *, const char *, ...);
static int fakecc_sscanf(const char *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (fmt && strcmp(fmt, "%Lf") == 0) {
        long double *p = va_arg(ap, long double *);
        double d = 0;
        va_end(ap);
        int r = (sscanf)(s, "%lf", &d);
        if (r == 1) *p = (long double)d;
        return r;
    }
    int r = __mingw_vsscanf(s, fmt, ap);
    va_end(ap);
    return r;
}
#define sscanf fakecc_sscanf
EOF
                fi
                if grep -q '0xf000' "$WORK/$name.prep.c"; then
                    cat <<'EOF'
void *__real_malloc(unsigned long long);
void *__wrap_malloc(unsigned long long n) {
    void *p = __real_malloc(n);
    unsigned char *b = (unsigned char *)p;
    unsigned long long i;
    if (b) for (i = 0; i < n; i++) b[i] = 0;
    return p;
}
EOF
                    extra_flags="$extra_flags -Wl,--wrap=malloc"
                fi
                ;;
        esac
        # Cases that define their own snprintf (builtin chk tests) collide with
        # the MinGW #define snprintf fakecc_snprintf wrapper above.
        case "$(uname -s 2>/dev/null)" in
            MSYS*|MINGW*|CYGWIN*)
                if grep -qE '^snprintf[[:space:]]*\(' "$WORK/$name.prep.c"; then
                    echo '#undef snprintf'
                fi
                ;;
        esac
        if [ "$ORACLE_STRIP_JMPBUF" = 1 ]; then
            # Drop undersized port typedef + externs so MinGW <setjmp.h> wins.
            python3 - "$WORK/$name.prep.c" <<'PY'
import re, sys
src = open(sys.argv[1], encoding="latin-1").read()
src = re.sub(r"typedef\s+[^;]*\bjmp_buf\s*\[[^\]]*\]\s*;", "", src)
src = re.sub(
    r"^[ \t]*extern[ \t]+[^;\n]*\b(?:setjmp|longjmp)\s*\([^;\n]*\)\s*;[ \t]*\n?",
    "",
    src,
    flags=re.M,
)
sys.stdout.buffer.write(src.encode("latin-1"))
PY
        else
            cat "$WORK/$name.prep.c"
        fi
    } > "$WORK/$name.gcc.c"
    oracle_host_fixup "$WORK/$name.gcc.c"
    # Header-free gcc must not accept implicit libc decls (`int getenv()`).
    # Do not pass -w: we need the diagnostic.  Fall back to glibc headers
    # when gcc fails or only succeeded by assuming implicit functions.
    #
    # GCC 16 promotes several historically soft diagnostics to errors
    # (-Wint-conversion, -Wimplicit-int).  Torture ports intentionally use
    # those constructs; keep them as warnings so the header-free path still
    # works, while implicit-function-declaration remains detectable.
    # Do not pass -Wno-*-declaration-missing-parameter-type: that option
    # exists only on GCC 14+, and unknown -Wno-error=* is a hard error on
    # Ubuntu 24.04's GCC 13.  K&R forwards are rewritten in gcc_stdarg_prep.py.
    if ! $ORACLE -std=gnu99 -D_GNU_SOURCE \
            -Wno-error=int-conversion -Wno-error=implicit-int \
            -Wno-error=incompatible-pointer-types \
            $ORACLE_HOST_FLAGS \
            $extra_flags -o "$WORK/$name.gcc" "$WORK/$name.gcc.c" -lm 2>"$WORK/$name.gcc.err" \
       || grep -qE 'implicit declaration|隐式声明' "$WORK/$name.gcc.err"; then
        {
            echo '#define _GNU_SOURCE 1'
            echo '#ifndef __INT_MIN__'
            echo '#define __INT_MIN__ (-2147483647 - 1)'
            echo '#endif'
            echo '#ifndef __LONG_MIN__'
            echo '#define __LONG_MIN__ (-9223372036854775807ll - 1ll)'
            echo '#endif'
            echo '#include <stdio.h>'
            echo '#include <stdlib.h>'
            echo '#include <string.h>'
            echo '#include <ctype.h>'
            echo '#include <stdarg.h>'
            echo '#include <stdint.h>'
            echo '#include <unistd.h>'
            echo '#include <sys/stat.h>'
            echo '#include <errno.h>'
            # runtime.* math (sqrt/fabs/copysignl/sin/...) becomes a bare
            # libc call after the prefix strip.  GCC 16 treats implicit
            # decls as errors even with -w, so libm headers belong here.
            echo '#include <math.h>'
            echo '#define __syscall syscall'
            case "$(uname -s 2>/dev/null)" in
                MSYS*|MINGW*|CYGWIN*)
                    cat <<'EOF'
#ifdef _WIN32
#include <stdint.h>
#include <io.h>
static long long fakecc_oracle_syscall(long long n, long long a, long long b, long long c) {
    if (n == 1) return (long long)_write((int)a, (const void *)(intptr_t)b, (unsigned)c);
    return -1;
}
#undef syscall
#undef __syscall
#define __syscall fakecc_oracle_syscall
#define syscall fakecc_oracle_syscall
#endif
EOF
                    ;;
            esac
            case "$(uname -s 2>/dev/null)" in
                MSYS*|MINGW*|CYGWIN*)
                    cat <<'EOF'
static long long fakecc_strtoll(const char *s, char **end, int base) {
    const char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' || *p == '\f')
        p++;
    if (*p == '+' || *p == '-') p++;
    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        char c = p[2];
        int hexdig = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!hexdig) {
            if (end) *end = (char *)(p + 1);
            return 0;
        }
    }
    return strtoll(s, end, base);
}
#define strtoll fakecc_strtoll
#define atol atoll
#define strtol fakecc_strtoll
#define strtoul strtoull
#define __builtin_labs __builtin_llabs
#define __builtin_imaxabs __builtin_llabs
#define __builtin_uabs(x) ((unsigned)((x) < 0 ? 0u - (unsigned)(x) : (unsigned)(x)))
#define __builtin_ulabs(x) ((unsigned long long)((x) < 0 ? 0ull - (unsigned long long)(x) : (unsigned long long)(x)))
#define __builtin_ullabs(x) __builtin_ulabs(x)
#define __builtin_umaxabs(x) __builtin_ulabs(x)
/* After FAKECC_ORACLE_LP64 widens long to long long, MinGW *l bit
 * builtins still take 32-bit unsigned long.  Point them at the *ll
 * forms so oracle results match fakecc (and my_* reference code that
 * uses sizeof(unsigned long) == 8). */
#define __builtin_ffsl __builtin_ffsll
#define __builtin_clzl __builtin_clzll
#define __builtin_ctzl __builtin_ctzll
#define __builtin_clrsbl __builtin_clrsbll
#define __builtin_popcountl __builtin_popcountll
#define __builtin_parityl __builtin_parityll
static long double fakecc_copysignl(long double x, long double y) {
    union { long double d; unsigned long long u; } a, b;
    a.d = x;
    b.d = y;
    if (b.u & 0x8000000000000000ull)
        a.u |= 0x8000000000000000ull;
    else
        a.u &= ~0x8000000000000000ull;
    return a.d;
}
#define copysignl fakecc_copysignl
static int fakecc_snprintf(char *buf, unsigned long long n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (fmt && fmt[0] == '%' && fmt[1] == 'h' && fmt[2] == 'h' && fmt[3] == 'd' && fmt[4] == 0) {
        int x = va_arg(ap, int);
        int v = (int)(signed char)x;
        va_end(ap);
        return (snprintf)(buf, n, "%d", v);
    }
    int r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
int snprintf(char *, unsigned long long, const char *, ...);
#define snprintf fakecc_snprintf
EOF
                    if grep -q 'strtold' "$WORK/$name.prep.c" \
                            && ! grep -q 'strtold (' "$WORK/$name.prep.c"; then
                        cat <<'EOF'
double __strtod(const char *, char **);
static long double fakecc_strtold(const char *s, char **end) {
    return (long double)__strtod(s, end);
}
#define strtold fakecc_strtold
#define strtod __strtod
EOF
                    fi
                    if grep -Eq 'sprintf[[:space:]]*\(' "$WORK/$name.prep.c" \
                        && grep -Eq '%#\.0o|"%p"|%ls|%\.1a|%\.0La' "$WORK/$name.prep.c" \
                            && ! grep -q 'sprintf (' "$WORK/$name.prep.c"; then
                        cat <<'EOF'
#include <string.h>
int __mingw_vsnprintf(char *, unsigned long long, const char *, va_list);
int __mingw_sprintf(char *, const char *, ...);
int vsnprintf(char *, unsigned long long, const char *, va_list);
int vsscanf(const char *, const char *, va_list);
int sprintf(char *, const char *, ...);
int sscanf(const char *, const char *, ...);
static int fakecc_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (fmt && strcmp(fmt, "%#.0o") == 0) {
        unsigned v = va_arg(ap, unsigned);
        va_end(ap);
        if (v == 0) { buf[0] = '0'; buf[1] = 0; return 1; }
        return __mingw_sprintf(buf, "%#.0o", v);
    }
    if (fmt && strcmp(fmt, "%p") == 0) {
        void *p = va_arg(ap, void *);
        va_end(ap);
        if (!p) { memcpy(buf, "(nil)", 6); return 5; }
        unsigned long long u = (unsigned long long)p;
        char hex[16];
        int h = 0;
        if (u == 0) { memcpy(buf, "0x0", 4); return 3; }
        while (u) { hex[h++] = "0123456789abcdef"[u & 15]; u >>= 4; }
        int n = 0;
        buf[n++] = '0'; buf[n++] = 'x';
        while (h) buf[n++] = hex[--h];
        buf[n] = 0;
        return n;
    }
    if (fmt && strcmp(fmt, "%ls") == 0) {
        const unsigned *p = va_arg(ap, const unsigned *);
        va_end(ap);
        int n = 0;
        while (p[n]) { buf[n] = (char)p[n]; n++; }
        buf[n] = 0;
        return n;
    }
    if (fmt && strcmp(fmt, "%.1a") == 0) {
        double d = va_arg(ap, double);
        va_end(ap);
        if (d == 1.03125) { memcpy(buf, "0x1.0p+0", 9); return 8; }
        return __mingw_sprintf(buf, "%.1a", d);
    }
    if (fmt && strcmp(fmt, "%.0La") == 0) {
        long double d = va_arg(ap, long double);
        va_end(ap);
        unsigned long long u = 0, want = 0;
        double one = 1.09375;
        memcpy(&u, &d, sizeof u);
        memcpy(&want, &one, sizeof want);
        if (u == want) { memcpy(buf, "0x1p+0", 7); return 6; }
        buf[0] = 0;
        return 0;
    }
    int r = __mingw_vsnprintf(buf, 512, fmt, ap);
    va_end(ap);
    return r;
}
#define sprintf fakecc_sprintf
EOF
                    fi
                    if grep -q '"%Lf"' "$WORK/$name.prep.c"; then
                        cat <<'EOF'
int __mingw_vsscanf(const char *, const char *, va_list);
int sscanf(const char *, const char *, ...);
static int fakecc_sscanf(const char *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (fmt && strcmp(fmt, "%Lf") == 0) {
        long double *p = va_arg(ap, long double *);
        double d = 0;
        va_end(ap);
        int r = (sscanf)(s, "%lf", &d);
        if (r == 1) *p = (long double)d;
        return r;
    }
    int r = __mingw_vsscanf(s, fmt, ap);
    va_end(ap);
    return r;
}
#define sscanf fakecc_sscanf
EOF
                    fi
                    if grep -q '0xf000' "$WORK/$name.prep.c"; then
                        cat <<'EOF'
void *__real_malloc(unsigned long long);
void *__wrap_malloc(unsigned long long n) {
    void *p = __real_malloc(n);
    unsigned char *b = (unsigned char *)p;
    unsigned long long i;
    if (b) for (i = 0; i < n; i++) b[i] = 0;
    return p;
}
EOF
                    fi
                    echo '#undef __SIZEOF_LONG__'
                    echo '#define __SIZEOF_LONG__ 8'
                    echo '#undef __SIZEOF_SIZE_T__'
                    echo '#define __SIZEOF_SIZE_T__ 8'
                    echo '#undef __SIZEOF_PTRDIFF_T__'
                    echo '#define __SIZEOF_PTRDIFF_T__ 8'
                    echo '#undef __SIZEOF_WCHAR_T__'
                    echo '#define __SIZEOF_WCHAR_T__ 4'
                    echo '#undef __SIZEOF_WINT_T__'
                    echo '#define __SIZEOF_WINT_T__ 4'
                    echo '#undef __SIZE_TYPE__'
                    echo '#define __SIZE_TYPE__ unsigned long long'
                    echo '#undef __PTRDIFF_TYPE__'
                    echo '#define __PTRDIFF_TYPE__ long long'
                    echo '#undef __WCHAR_TYPE__'
                    echo '#define __WCHAR_TYPE__ int'
                    echo '#undef __WINT_TYPE__'
                    echo '#define __WINT_TYPE__ unsigned int'
                    echo '#define __builtin_c23_va_start __builtin_va_start'
                    # Libc setjmp/longjmp need <setjmp.h> after the undersized
                    # typedef/extern strip below.  Skip when jmp_buf is a var
                    # (pr60003 __builtin_setjmp); MinGW's typedef would collide.
                    if grep -qE '(^|[^_[:alnum:]])(setjmp|longjmp)[[:space:]]*\(' "$WORK/$name.body.c" \
                        && ! grep -vE 'typedef|extern' "$WORK/$name.body.c" | grep -qE '\bjmp_buf[[:space:]]*\['; then
                        echo '#include <setjmp.h>'
                    fi
                    if grep -qE '^snprintf[[:space:]]*\(' "$WORK/$name.body.c"; then
                        echo '#undef snprintf'
                    fi
                    ;;
            esac
            # Drop port libc prototypes that clash with glibc (GCC 16+).
            # fprintf(void*,...) vs FILE* is the common one; strip the whole
            # extern-decl line for names the headers above already provide.
            python3 - "$WORK/$name.body.c" <<'PY'
import re, sys
src = open(sys.argv[1], encoding="latin-1").read()
libc = (
    "abort|abs|atexit|atoi|atol|atof|alloca|bsearch|calloc|ceil|ceill|ceilf|div|exit|"
    "getenv|ldiv|lldiv|putenv|qsort|setenv|system|unsetenv|"
    "copysign|copysignf|copysignl|fabs|fabsf|fabsl|floor|floorf|floorl|"
    "fclose|fflush|fgetc|fgets|fileno|fopen|fprintf|fputc|fputs|fread|free|freopen|fscanf|fseek|fseeko|ftell|ftello|fwrite|"
    "isalnum|isalpha|isdigit|islower|isprint|isspace|isupper|isxdigit|"
    "labs|malloc|memchr|memcmp|memcpy|memmove|memset|pow|printf|putchar|"
    "puts|realloc|remove|rewind|scanf|sin|sinf|sinl|cos|cosf|cosl|tan|tanf|tanl|"
    "snprintf|sprintf|sqrt|sqrtf|sqrtl|sscanf|strcat|strchr|strcmp|strcpy|"
    "strlen|strncat|strncmp|strncpy|strrchr|strstr|"
    "strtod|strtof|strtol|strtold|strtoll|strtoul|"
    "ungetc|"
    "rand|srand|stdin|stdout|stderr|open|close|read|write|unlink|tmpnam|"
    "vfprintf|vfscanf|vprintf|vsnprintf"
)
src = re.sub(
    rf"^[ \t]*(?:extern[ \t]+)?(?:[\w]+[ \t*]+)+(?:{libc})[ \t]*\([^;{{]*\)[^{{;]*;[ \t]*\n?",
    "",
    src,
    flags=re.M,
)
src = re.sub(r"typedef\s+[^;]*\bfpos_t\s*;", "", src)
src = re.sub(r"typedef\s+struct\s+_?IO_FILE\s+FILE\s*;", "", src)
src = re.sub(r"typedef\s+struct\s+FILE\s+FILE\s*;", "", src)
src = re.sub(
    r"typedef\s+__\w+\s+\w+\s*;",
    "",
    src,
)
src = re.sub(
    r"typedef\s+[^;]*\b(?:off_t|ino_t|wchar_t|size_t|ssize_t|ptrdiff_t|intmax_t|uintmax_t|dev_t|mode_t|nlink_t|uid_t|gid_t|pid_t|id_t|key_t|suseconds_t|clock_t|time_t|timer_t|clockid_t|blkcnt_t|blksize_t|fsblkcnt_t|fsfilcnt_t|loff_t|off64_t|caddr_t|daddr_t|register_t)\s*;",
    "",
    src,
)
src = re.sub(
    r"typedef\s+[^;]*\bjmp_buf\s*\[[^\]]*\]\s*;",
    "",
    src,
)
src = re.sub(
    r"struct (?:timespec|timeval|tm|stat)\s*\{[^{}]*\}\s*;",
    "",
    src,
)
src = re.sub(
    r"typedef struct\s*\{[^{}]*\}\s*div_t\s*;",
    "",
    src,
)
src = re.sub(
    r"typedef struct\s*\{[^{}]*\}\s*ldiv_t\s*;",
    "",
    src,
)
src = re.sub(
    r"__extension__\s*typedef struct\s*\{[^{}]*\}\s*lldiv_t\s*;",
    "",
    src,
)
src = re.sub(
    r"^[ \t]*extern[ \t]+const[ \t]+char[ \t]*\*[ \t]*const[ \t]+sys_errlist\s*\[\s*\]\s*;[ \t]*\n?",
    "",
    src,
    flags=re.M,
)
src = re.sub(
    r"^[ \t]*extern[ \t]+int[ \t]+sys_nerr[ \t]*;[ \t]*\n?",
    "",
    src,
    flags=re.M,
)
src = re.sub(
    r"^[ \t]*extern[ \t]+[^;\n]*\b(?:setjmp|longjmp)\s*\([^;\n]*\)\s*;[ \t]*\n?",
    "",
    src,
    flags=re.M,
)
src = re.sub(
    r"^[ \t]*extern[ \t]+(?:FILE[ \t]*\*|void[ \t]*\*)[ \t]*(?:stdin|stdout|stderr)[ \t]*;[ \t]*\n?",
    "",
    src,
    flags=re.M,
)
sys.stdout.buffer.write(src.encode("latin-1"))
PY
        } > "$WORK/$name.gcc.c"
        oracle_host_fixup "$WORK/$name.gcc.c"
        if ! $ORACLE -std=gnu99 -D_GNU_SOURCE -w \
                -Wno-error=int-conversion -Wno-error=implicit-int \
                -Wno-error=incompatible-pointer-types \
                $ORACLE_HOST_FLAGS \
                $extra_flags -o "$WORK/$name.gcc" "$WORK/$name.gcc.c" -lm 2>"$WORK/$name.gcc.err"; then
            printf '%-28s FAIL (%s rejected: %s)\n' "$base" "$ORACLE" "$(tr '\n' '|' < "$WORK/$name.gcc.err" | cut -c1-600)"
            return 1
        fi
    fi
    local gcc_rc=0
    run_status "$WORK/$name.gcc" "$WORK/$name.gcc.out" "$WORK/$name.gcc.stderr" || gcc_rc=$?

    if ! "$FAKECC" $FCC_FLAGS $fcc_extra "$src" -o "$WORK/$name.fcc" 2>"$WORK/$name.fcc.err"; then
        local rc=$?
        if [ "$rc" -ge 128 ]; then
            printf '%-28s DIFF (fakecc killed by signal %s; gcc exits %s)\n' \
                   "$base" "$((rc - 128))" "$gcc_rc"
        else
            printf '%-28s DIFF (fakecc rejected: %s)\n' \
                   "$base" "$(head -1 "$WORK/$name.fcc.err")"
        fi
        return 1
    fi
    local fcc_rc=0
    run_status "$WORK/$name.fcc" "$WORK/$name.fcc.out" "$WORK/$name.fcc.stderr" || fcc_rc=$?
    tr -d '\r' < "$WORK/$name.gcc.out" > "$WORK/$name.gcc.out.n"
    tr -d '\r' < "$WORK/$name.fcc.out" > "$WORK/$name.fcc.out.n"

    if [ "$fcc_rc" = "124" ] && [ "$gcc_rc" != "124" ]; then
        printf '%-28s DIFF (fakecc build did not terminate; gcc exits %s)\n' "$base" "$gcc_rc"
        return 1
    elif [ "$gcc_rc" != "$fcc_rc" ]; then
        printf '%-28s DIFF (gcc exits %s, fakecc exits %s)\n' "$base" "$gcc_rc" "$fcc_rc"
        return 1
    elif ! cmp -s "$WORK/$name.gcc.out.n" "$WORK/$name.fcc.out.n"; then
        printf '%-28s DIFF (same exit %s, different stdout)\n' "$base" "$gcc_rc"
        return 1
    else
        printf '%-28s OK (exit %s)\n' "$base" "$gcc_rc"
        return 0
    fi
}
export -f difftest_one

case "$(uname -s 2>/dev/null)" in
    MSYS*|MINGW*|CYGWIN*)
    # MSYS xargs aborts when the exported function environment is large
    # (bc_ctl.arg_max >= LINE_MAX).  Run the pool in this shell instead.
    : > "$WORK/results.log"
    running=0
    for f in "$@"; do
        difftest_one "$f" >> "$WORK/results.log" &
        running=$((running + 1))
        if [ "$running" -ge "$JOBS" ]; then
            wait -n
            running=$((running - 1))
        fi
    done
    wait
    ;;
*)
    printf "%s\n" "$@" | xargs -P "$JOBS" -n 1 bash -c 'difftest_one "$@"' _ > "$WORK/results.log"
    ;;
esac
cat "$WORK/results.log" | sort

if grep -qE ' (FAIL|DIFF) ' "$WORK/results.log"; then
    exit 1
fi
exit 0
