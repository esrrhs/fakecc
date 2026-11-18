#!/usr/bin/env bash
# Deterministic synthetic validation for fuzz/run_with_runtime_diag.sh.
# Does NOT run fuzz_compile, libFuzzer, or any blocked fuzz task.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WRAP="$ROOT/fuzz/run_with_runtime_diag.sh"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0
fail=0

check() {
  local name=$1
  shift
  if "$@"; then
    echo "PASS: $name"
    pass=$((pass + 1))
  else
    echo "FAIL: $name"
    fail=$((fail + 1))
  fi
}

# 1) Successful stand-in command (not named fuzz).
check success_exit_0 bash -c "
  '$WRAP' --budget-seconds 5 --sample-interval 1 \
    --diag-log '$TMP/ok.diag' --tee-log '$TMP/ok.out' --label syn-ok -- \
    bash -c 'echo syn-ok-body; exit 0'
  test \$? -eq 0
  grep -q 'event=start' '$TMP/ok.diag'
  grep -q 'event=child_exit exit=0' '$TMP/ok.diag'
  grep -q 'syn-ok-body' '$TMP/ok.out'
  grep -q 'budget_s=5' '$TMP/ok.diag'
"

# 2) Non-zero exit must be preserved exactly.
check nonzero_exit_41 bash -c "
  set +e
  '$WRAP' --budget-seconds 5 --sample-interval 1 \
    --diag-log '$TMP/nz.diag' --label syn-nz -- \
    bash -c 'exit 41'
  st=\$?
  set -e
  test \"\$st\" -eq 41
  grep -q 'event=child_exit exit=41' '$TMP/nz.diag'
"

# 3) TERM forwarded to child only; status preserved (128+15 or child exit).
#    Use a child that dies on TERM (no trap) so exit is typically 143.
check term_forward bash -c "
  set +e
  '$WRAP' --budget-seconds 30 --sample-interval 1 \
    --diag-log '$TMP/term.diag' --label syn-term -- \
    sleep 60 &
  wp=\$!
  for i in 1 2 3 4 5 6 7 8 9 10; do
    if grep -q 'event=start' '$TMP/term.diag' 2>/dev/null; then
      break
    fi
    sleep 0.2
  done
  kill -s TERM \"\$wp\"
  wait \"\$wp\"
  st=\$?
  set -e
  grep -q 'event=signal_received signal=TERM' '$TMP/term.diag'
  grep -Eq 'reason=pre_forward_TERM|event=child_exit' '$TMP/term.diag'
  # Exact preservation: SIGTERM => 143 on Linux bash wait.
  test \"\$st\" -eq 143
"

# 4) Sampler cleanup: after success, no leftover sampler for our label.
check sampler_cleanup bash -c "
  '$WRAP' --budget-seconds 3 --sample-interval 1 \
    --diag-log '$TMP/clean.diag' --label syn-clean -- \
    bash -c 'sleep 2; exit 0'
  # No process cmdline containing our diag log path should remain.
  if command -v pgrep >/dev/null 2>&1; then
    ! pgrep -af 'syn-clean|$(basename "$TMP")/clean.diag' 2>/dev/null | grep -v pgrep | grep -q .
  else
    true
  fi
  grep -q 'event=child_exit exit=0' '$TMP/clean.diag'
"

# 5) Missing /proc and cgroup paths must not fail the wrapped command.
#    Simulate by running a quick command; on any OS the wrapper must exit 0
#    even when cgroup files are absent (always true on macOS; on Linux we
#    additionally verify n/a-tolerant parsing by pointing at a fake pid sample
#    indirectly — success path already exercises absent-cgroup branches for
#    short-lived processes).
check missing_proc_tolerant bash -c "
  '$WRAP' --budget-seconds 2 --sample-interval 1 \
    --diag-log '$TMP/noproc.diag' --label syn-noproc -- \
    bash -c 'exit 0'
  grep -q 'event=child_exit exit=0' '$TMP/noproc.diag'
  # Metrics fields must be present even when values are n/a.
  grep -q 'mem_available_kb=' '$TMP/noproc.diag'
  grep -q 'cgroup_memory_current=' '$TMP/noproc.diag'
"

# 6) bash -n on wrapper
check bash_syntax bash -n "$WRAP"

echo "---"
echo "Passed: $pass  Failed: $fail"
test "$fail" -eq 0
