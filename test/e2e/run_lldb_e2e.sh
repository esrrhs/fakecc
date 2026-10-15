#!/usr/bin/env bash
# LLDB end-to-end tests: drive a real lldb against fakecc output and check what
# it reports. This is the macOS equivalent of run_gdb_e2e.sh.
#
# Cases live in cases/debug/*.c and are annotated:
#
#     // expect: 42                 program's own exit code
#     // lldb: breakpoint set -f {src} -l {line}   an lldb command
#     // lldb: run                   run the program
#     // lldb: frame variable s      print variable s
#     // lldb_expect: \(int\) s = 42 extended regex the output must contain
#     // lldb_reject: optimized out  extended regex the output must NOT contain
#
# For compatibility, we also support the gdb: annotations and translate them
# to lldb commands automatically.
set -uo pipefail
_COMPAT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
. "$_COMPAT_DIR/compat.sh" || { echo "cannot source compat.sh from $_COMPAT_DIR" >&2; exit 2; }
export -f timeout nproc 2>/dev/null || true


ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FAKECC="${1:-$ROOT/build/fakecc}"
shift || true
CC_EXTRA="${CC_FLAGS:-}"
CASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/cases/debug"
CC_TIMEOUT=${CC_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-20}

if ! command -v lldb >/dev/null 2>&1; then
  echo "skip: lldb not installed"
  exit 0
fi

# Skip on Linux - we use gdb there
HOST_OS=$(uname -s | tr 'A-Z' 'a-z')
if [ "$HOST_OS" = "linux" ]; then
  echo "skip: lldb suite is macOS-only (host $HOST_OS)"
  exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

FAIL=0
n_pass=0 n_fail=0

read_annotations() {
  local src="$1" key="$2"
  sed -n "s|^// ${key}: \{0,1\}\(.*\)\$|\1|p" "$src"
}

run_case() {
  local src="$1" flags="$2"
  local name="$(basename "$src")"
  local label="${flags:-default}"
  local bin="$TMP/prog"

  rm -f "$bin"
  if ! timeout "$CC_TIMEOUT" "$FAKECC" $flags -g "$src" -o "$bin" 2>"$TMP/cc.err"; then
    echo "FAIL $name [$label] (compile failed: $(head -1 "$TMP/cc.err"))"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  local expect got=0
  expect=$(read_annotations "$src" "expect" | head -1)
  if [ -n "$expect" ]; then
    # `! cmd` leaves the *negated* status in $?, so `if ! cmd; then got=$?`
    # records 0 for a program that exited 42 and fails every case whose
    # expected exit status is non-zero.  Read the status from the command
    # itself, the way run_gdb_e2e.sh does.
    timeout "$RUN_TIMEOUT" "$bin" >"$TMP/run.out" 2>"$TMP/run.err" || got=$?
    if [ "$got" != "$expect" ]; then
      echo "FAIL $name [$label] (exit $got, expected $expect)"
      n_fail=$((n_fail + 1)); FAIL=1; return
    fi
  fi

  # Build lldb command script
  local lldb_script="$TMP/lldb.cmd"
  > "$lldb_script"
  echo "settings set auto-confirm true" >> "$lldb_script"

  # Translate gdb: annotations to lldb commands, or use lldb: annotations directly
  local has_lldb_cmds=0
  while IFS= read -r cmd; do
    has_lldb_cmds=1
    # Translate GDB commands to LLDB
    case "$cmd" in
      "break {brk}")
        local brk_line
        brk_line=$(grep -n '// BRK' "$src" | head -1 | cut -d: -f1)
        echo "breakpoint set -f $src -l $brk_line" >> "$lldb_script"
        ;;
      "break "*)
        echo "breakpoint set --name ${cmd#break }" >> "$lldb_script"
        ;;
      "run")
        echo "run" >> "$lldb_script"
        ;;
      "print "*)
        local var="${cmd#print }"
        # gdb's `print` evaluates an arbitrary expression; lldb's
        # `frame variable` only prints variables, and chokes on anything
        # else (`sizeof(p)`).  `expression` is the real counterpart.
        echo "expression -- $var" >> "$lldb_script"
        ;;
      "info args"|"info locals"|"info scope "*)
        # lldb has no `info args`: `frame variable` with no operand lists
        # the frame's arguments and locals together, which is what the
        # `a = 40` / `v = 21` expectations are looking for.
        echo "frame variable" >> "$lldb_script"
        ;;
      "continue")
        echo "continue" >> "$lldb_script"
        ;;
      "continue "*)
        # gdb's `continue N` ignores the current breakpoint N more times,
        # so it stops on the (N+1)th hit.  lldb has no ignore count on the
        # command; N plain continues land on the same hit.
        local n="${cmd#continue }"
        case "$n" in
          ''|*[!0-9]*) echo "continue" >> "$lldb_script" ;;
          *) local k=0; while [ "$k" -lt "$n" ]; do
               echo "continue" >> "$lldb_script"; k=$((k + 1)); done ;;
        esac
        ;;
      "delete "*)
        echo "breakpoint delete ${cmd#delete }" >> "$lldb_script"
        ;;
      "backtrace"|"bt")
        echo "thread backtrace" >> "$lldb_script"
        ;;
      *)
        echo "$cmd" >> "$lldb_script"
        ;;
    esac
  done < <(read_annotations "$src" "gdb")

  # Also process lldb: annotations
  while IFS= read -r cmd; do
    has_lldb_cmds=1
    echo "$cmd" >> "$lldb_script"
  done < <(read_annotations "$src" "lldb")

  if [ "$has_lldb_cmds" = "0" ]; then
    echo "PASS $name [$label] (no debugger commands)"
    n_pass=$((n_pass + 1)); return
  fi

  echo "quit" >> "$lldb_script"

  # Run lldb with the script.  lldb exits non-zero as soon as any command in
  # the script fails -- `expression` on a variable that has no location, say --
  # which is a property of the program being debugged, not of lldb.  Treat a
  # non-zero status as ordinary output and let the expectations judge it; only
  # a timeout (124) means lldb itself did not come back.
  timeout "$RUN_TIMEOUT" lldb -b -s "$lldb_script" "$bin" >"$TMP/lldb.out" 2>"$TMP/lldb.err"
  local lldb_rc=$?
  if [ "$lldb_rc" = 124 ]; then
    echo "FAIL $name [$label] (lldb timed out)"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi
  if [ ! -s "$TMP/lldb.out" ]; then
    echo "FAIL $name [$label] (lldb produced no output: $(head -1 "$TMP/lldb.err"))"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  # The cases annotate `gdb_expect: \$1 = 8` and were first written against
  # gdb, so the two debuggers have to be made to agree on more than the
  # command names.  Everything below is a pure difference in how the same
  # information is spelled.
  #
  #   * lldb numbers persistent results from $0, gdb from $1.
  #   * lldb prints a C array as `([0] = 6, [1] = 9)`, gdb as `{6, 9}`.
  #   * lldb prints a char as `'A'`, gdb as `65 'A'`.
  #   * lldb prints a bool as `true`, gdb as `1`.
  #   * lldb writes a frame as
  #         frame #1: 0x100003f00 prog`middle(v=39) at f.c:19:36
  #     gdb as
  #         #1  0x0000555555555189 in middle (v=39) at f.c:19
  #   * lldb announces a stop with `stop reason = breakpoint 1.1` plus the
  #     frame; gdb with one `Breakpoint 1, main () at f.c:21` banner.
  #   * lldb reports a resolved breakpoint as
  #         Breakpoint 1: where = prog`scale + 12 at f.c:18:12, address = 0x...
  #     gdb as
  #         Breakpoint 1 at 0x...: file f.c, line 18.
  #
  # Rewriting the captured output keeps one copy of the expectations instead
  # of a gdb pair and an lldb pair in every case file.
  perl -0777 -pi -e '
    my @out;
    my $brk;                      # breakpoint we just stopped at, if any
    for my $ln (split /\n/, $_) {
      # lldb shows several lines of source around the stop, which can include
      # the annotations themselves; `gdb_reject: Backtrace stopped` must not
      # match just because the comment is echoed back.
      # lldb pads the line number out to a column, so it is followed by
      # spaces and *then* a tab -- `^\s*\d+\t` looks right and matches nothing.
      next if $ln =~ /^\s*\d+\s+\/\/\s*(?:gdb|lldb)_/;

      $brk = $1 if $ln =~ /stop reason = breakpoint (\d+)\./;

      # "Breakpoint 1: where = prog`scale + 12 at f.c:18:12, address = 0xADDR"
      # becomes the gdb form "Breakpoint 1 at 0xADDR: file f.c, line 18."
      $ln =~ s/^Breakpoint (\d+): where = \S+`[^ ]+ \+ \d+ at (\S+?):(\d+)(?::\d+)?, address = (0x[0-9a-f]+).*$/Breakpoint $1 at $4: file $2, line $3./;

      if ($ln =~ /^\s*\*?\s*frame #(\d+):\s+(0x[0-9a-f]+)\s+\S+`([A-Za-z_]\w*)\(([^)]*)\)(.*)$/) {
        my ($n,$addr,$fn,$args,$rest) = ($1,$2,$3,$4,$5);
        $ln = "#$n  " . ($n == 0 ? "" : "$addr in ") . "$fn ($args)$rest";
        if (defined $brk) {
          my $at = $rest =~ /(\s+at\s+\S+:\d+)/ ? $1 : "";
          push @out, "Breakpoint $brk, $fn ($args)$at";
          undef $brk;
        }
      } elsif ($ln =~ /^\s*\*?\s*frame #(\d+):\s+(0x[0-9a-f]+)\s+\S+`([A-Za-z_]\w*)(?:\s+at\s+(\S+):(\d+)(?::\d+)?)?\s*$/) {
        my ($n,$addr,$fn,$f,$l) = ($1,$2,$3,$4,$5);
        my $at = defined $f ? " at $f:$l" : "";
        $ln = "#$n  " . ($n == 0 ? "" : "$addr in ") . "$fn ()$at";
        if (defined $brk) {
          push @out, "Breakpoint $brk, $fn ()$at";
          undef $brk;
        }
      }

      if ($ln =~ /\(bool\)/) {
        $ln =~ s/=\s*true\s*$/= 1/;
        $ln =~ s/=\s*false\s*$/= 0/;
      }
      if ($ln =~ /=\s*'"'"'(.)'"'"'\s*$/) {
        my $o = ord($1);
        $ln =~ s/=\s*'"'"'(.)'"'"'\s*$/= $o '"'"'$1'"'"'/;
      }
      if ($ln =~ /= \(\[\d+\] = /) {
        $ln =~ s/= \(\[\d+\] = /= {/;
        $ln =~ s/\[\d+\] = //g;
        $ln =~ s/\)\s*$/}/;
      }
      $ln =~ s/\$([0-9]+)/"\$".($1+1)/ge;
      push @out, $ln;
    }
    $_ = join("\n", @out) . "\n";
  ' "$TMP/lldb.out" 2>/dev/null || {
    # No perl: at least shift the result numbering so the common cases work.
    sed -i.bak -e 's/\$4/$5/g' -e 's/\$3/$4/g' -e 's/\$2/$3/g' \
        -e 's/\$1/$2/g' -e 's/\$0/$1/g' "$TMP/lldb.out" 2>/dev/null || true
  }

  # Which pair of annotations applies.  The gdb_* pair is the default, since
  # every case was written against gdb and most read the same under lldb after
  # the rewriting above.  A case that carries any lldb_* annotation is one
  # where the two debuggers genuinely report different things and its lldb_*
  # pair replaces the gdb one rather than adding to it.
  local exp_key="gdb_expect" rej_key="gdb_reject"
  if [ -n "$(read_annotations "$src" "lldb_expect"; read_annotations "$src" "lldb_reject")" ]; then
    exp_key="lldb_expect"
    rej_key="lldb_reject"
  fi

  local expect_ok=1
  while IFS= read -r pattern; do
    if ! grep -qE "$pattern" "$TMP/lldb.out"; then
      echo "FAIL $name [$label] (expected pattern not found: $pattern)"
      echo "--- lldb output ---"
      cat "$TMP/lldb.out"
      echo "--- end ---"
      expect_ok=0
      break
    fi
  done < <(read_annotations "$src" "$exp_key")

  while IFS= read -r pattern; do
    if grep -qE "$pattern" "$TMP/lldb.out"; then
      echo "FAIL $name [$label] (rejected pattern found: $pattern)"
      echo "--- lldb output ---"
      cat "$TMP/lldb.out"
      echo "--- end ---"
      expect_ok=0
      break
    fi
  done < <(read_annotations "$src" "$rej_key")

  if [ "$expect_ok" = "1" ]; then
    echo "PASS $name [$label]"
    n_pass=$((n_pass + 1))
  else
    n_fail=$((n_fail + 1)); FAIL=1
  fi
}

if [ ! -d "$CASE_DIR" ] || [ -z "$(ls -A "$CASE_DIR"/*.c 2>/dev/null)" ]; then
  echo "skip: no debug test cases found"
  exit 0
fi

echo "=== lldb debug tests ==="
for src in "$CASE_DIR"/*.c; do
  run_case "$src" ""
  run_case "$src" "-O0"
  run_case "$src" "-O1"
done

echo "lldb: $n_pass passed, $n_fail failed"
exit $FAIL
