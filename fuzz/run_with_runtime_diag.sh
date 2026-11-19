#!/usr/bin/env bash
# Runtime observability wrapper for CI fuzz jobs.
#
# Diagnostic-only: does not change fuzzer args, env, duration, RSS limits, or
# sanitizer settings. Preserves the wrapped command's exact exit status
# (including signal deaths as 128+N when the shell reports them that way).
#
# Limitation: a GitHub Actions runner VM hard-kill (SIGKILL / abrupt host
# reclaim) bypasses traps and may prevent final samples and artifact upload.
set -u

usage() {
  cat <<'USAGE'
Usage: run_with_runtime_diag.sh [options] -- <command> [args...]

Options:
  --budget-seconds N   Declared fuzz budget in seconds (for elapsed vs budget)
  --diag-log PATH      Diagnostics log path (default: fuzz_runtime_diag.log)
  --tee-log PATH       Also tee command stdout/stderr to PATH (optional)
  --sample-interval N  Sample period in seconds (default: 30)
  --label NAME         Label written into diag lines (default: fuzz)
  -h, --help           Show help
USAGE
}

BUDGET_SECONDS=0
DIAG_LOG="fuzz_runtime_diag.log"
TEE_LOG=""
SAMPLE_INTERVAL=30
LABEL="fuzz"

while [ $# -gt 0 ]; do
  case "$1" in
    --budget-seconds)
      BUDGET_SECONDS=${2:?}
      shift 2
      ;;
    --diag-log)
      DIAG_LOG=${2:?}
      shift 2
      ;;
    --tee-log)
      TEE_LOG=${2:?}
      shift 2
      ;;
    --sample-interval)
      SAMPLE_INTERVAL=${2:?}
      shift 2
      ;;
    --label)
      LABEL=${2:?}
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "run_with_runtime_diag.sh: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [ $# -eq 0 ]; then
  echo "run_with_runtime_diag.sh: missing command after --" >&2
  usage >&2
  exit 2
fi

# Ensure parent dir for diag log exists when a path with directories is given.
diag_dir=$(dirname -- "$DIAG_LOG")
if [ "$diag_dir" != "." ]; then
  mkdir -p -- "$diag_dir" 2>/dev/null || true
fi
: >"$DIAG_LOG"

START_EPOCH=$(date +%s)
CHILD_PID=""
SAMPLER_PID=""
TERMINATING=0
LAST_SIGNAL=""

diag_line() {
  # Best-effort append; never abort the real command because of logging.
  printf '%s\n' "$*" >>"$DIAG_LOG" 2>/dev/null || true
}

iso_now() {
  date -u +"%Y-%m-%dT%H:%M:%SZ" 2>/dev/null || date
}

elapsed_seconds() {
  now=$(date +%s)
  echo $((now - START_EPOCH))
}

# Resolve nested cgroup v2 path for a PID when available (Linux only).
# Reads /proc/<pid>/cgroup and maps via /sys/fs/cgroup; falls back to
# common GHA paths. Read-only; never fails the wrapped command.
cgroup_dir_for_pid() {
  local pid=$1
  local cg_rel="" cg_path=""

  if [ ! -r "/proc/${pid}/cgroup" ]; then
    return 1
  fi

  # cgroup v2 unified: "0::/path"
  cg_rel=$(awk -F: '$1 == "0" { print $3; exit }' "/proc/${pid}/cgroup" 2>/dev/null || true)
  if [ -n "$cg_rel" ]; then
    cg_path="/sys/fs/cgroup${cg_rel}"
    if [ -d "$cg_path" ]; then
      printf '%s\n' "$cg_path"
      return 0
    fi
  fi

  # Fallbacks seen on GitHub-hosted Linux runners.
  for cand in \
    "/sys/fs/cgroup" \
    "/sys/fs/cgroup/user.slice" \
    "/sys/fs/cgroup/actions_job" \
    "/sys/fs/cgroup/docker"
  do
    if [ -r "${cand}/memory.current" ] || [ -r "${cand}/memory.stat" ]; then
      printf '%s\n' "$cand"
      return 0
    fi
  done
  return 1
}

read_file_trim() {
  local f=$1
  if [ -r "$f" ]; then
    # shellcheck disable=SC2002
    tr -d '\n' <"$f" 2>/dev/null || true
  fi
}

sample_once() {
  local pid=${1:-}
  local reason=${2:-periodic}
  local elap rss_kb rss_mb state mem_avail_kb mem_total_kb
  local cg_dir cg_current cg_max cg_events cg_peak
  local line

  elap=$(elapsed_seconds)
  rss_kb="n/a"
  rss_mb="n/a"
  state="n/a"
  mem_avail_kb="n/a"
  mem_total_kb="n/a"
  cg_current="n/a"
  cg_max="n/a"
  cg_events="n/a"
  cg_peak="n/a"

  if [ -n "$pid" ] && [ -r "/proc/${pid}/status" ]; then
    # VmRSS is in kB on Linux.
    rss_kb=$(awk '/^VmRSS:/ { print $2; exit }' "/proc/${pid}/status" 2>/dev/null || echo "n/a")
    state=$(awk '/^State:/ { print $2; exit }' "/proc/${pid}/status" 2>/dev/null || echo "n/a")
    if [ "$rss_kb" != "n/a" ] && [ -n "$rss_kb" ]; then
      rss_mb=$((rss_kb / 1024))
    fi
  elif [ -n "$pid" ] && command -v ps >/dev/null 2>&1; then
    # Portable-ish fallback (macOS/Windows MSYS): RSS often in KB already.
    rss_kb=$(ps -o rss= -p "$pid" 2>/dev/null | tr -d ' ' || true)
    if [ -n "${rss_kb:-}" ]; then
      rss_mb=$((rss_kb / 1024))
    else
      rss_kb="n/a"
    fi
  fi

  if [ -r /proc/meminfo ]; then
    mem_avail_kb=$(awk '/^MemAvailable:/ { print $2; exit }' /proc/meminfo 2>/dev/null || echo "n/a")
    mem_total_kb=$(awk '/^MemTotal:/ { print $2; exit }' /proc/meminfo 2>/dev/null || echo "n/a")
  fi

  if [ -n "$pid" ]; then
    cg_dir=$(cgroup_dir_for_pid "$pid" 2>/dev/null || true)
    if [ -n "${cg_dir:-}" ]; then
      cg_current=$(read_file_trim "${cg_dir}/memory.current")
      [ -n "$cg_current" ] || cg_current="n/a"
      cg_max=$(read_file_trim "${cg_dir}/memory.max")
      [ -n "$cg_max" ] || cg_max="n/a"
      cg_peak=$(read_file_trim "${cg_dir}/memory.peak")
      [ -n "$cg_peak" ] || cg_peak="n/a"
      if [ -r "${cg_dir}/memory.events" ]; then
        cg_events=$(tr '\n' ' ' <"${cg_dir}/memory.events" 2>/dev/null | sed 's/[[:space:]]*$//')
        [ -n "$cg_events" ] || cg_events="n/a"
      fi
    fi
  fi

  line="ts=$(iso_now) label=${LABEL} reason=${reason} pid=${pid:-none} elapsed_s=${elap} budget_s=${BUDGET_SECONDS} rss_kb=${rss_kb} rss_mb=${rss_mb} state=${state} mem_available_kb=${mem_avail_kb} mem_total_kb=${mem_total_kb} cgroup_memory_current=${cg_current} cgroup_memory_max=${cg_max} cgroup_memory_peak=${cg_peak} cgroup_memory_events=${cg_events}"
  diag_line "$line"
  # Also emit a short GHA-visible breadcrumb (not secrets; public metrics only).
  echo "[fuzz-diag] ${line}"
}

sampler_loop() {
  local pid=$1
  while true; do
    # Stop when the watched PID is gone. kill -0 works on Linux/macOS/MSYS;
    # missing /proc or cgroup must never fail the real command.
    if [ -z "$pid" ] || ! kill -0 "$pid" 2>/dev/null; then
      break
    fi
    sample_once "$pid" "periodic" || true
    sleep "$SAMPLE_INTERVAL" || break
  done
}

cleanup_sampler() {
  if [ -n "${SAMPLER_PID:-}" ]; then
    kill "$SAMPLER_PID" 2>/dev/null || true
    # Reap without failing.
    wait "$SAMPLER_PID" 2>/dev/null || true
    SAMPLER_PID=""
  fi
}

on_signal() {
  local sig=$1
  LAST_SIGNAL=$sig
  TERMINATING=1
  diag_line "ts=$(iso_now) label=${LABEL} event=signal_received signal=${sig} child_pid=${CHILD_PID:-none} elapsed_s=$(elapsed_seconds) budget_s=${BUDGET_SECONDS}"
  echo "[fuzz-diag] signal=${sig} received; forwarding to child pid=${CHILD_PID:-none} only (not process group)"
  if [ -n "${CHILD_PID:-}" ]; then
    sample_once "$CHILD_PID" "pre_forward_${sig}" || true
    # Forward to the fuzzer PID only — never kill -- -PGID / process groups.
    kill -s "$sig" "$CHILD_PID" 2>/dev/null || true
  fi
}

# Install traps before starting the child.
trap 'on_signal TERM' TERM
trap 'on_signal INT' INT

diag_line "ts=$(iso_now) label=${LABEL} event=start budget_s=${BUDGET_SECONDS} sample_interval_s=${SAMPLE_INTERVAL} note=runner_vm_hard_kill_may_bypass_traps_and_artifact_upload"
diag_line "ts=$(iso_now) label=${LABEL} event=platform uname=$(uname -s 2>/dev/null || echo unknown)"

# Start the wrapped command in the background in the SAME process group so
# GitHub's job cancellation still delivers signals to us; we then forward to
# the child PID only.
if [ -n "$TEE_LOG" ]; then
  tee_dir=$(dirname -- "$TEE_LOG")
  if [ "$tee_dir" != "." ]; then
    mkdir -p -- "$tee_dir" 2>/dev/null || true
  fi
  "$@" > >(tee -a -- "$TEE_LOG") 2>&1 &
else
  "$@" &
fi
CHILD_PID=$!

sample_once "$CHILD_PID" "post_start" || true

# Sampler watches only CHILD_PID.
sampler_loop "$CHILD_PID" &
SAMPLER_PID=$!

set +e
wait "$CHILD_PID"
CHILD_STATUS=$?
# If a trapped signal interrupted wait, re-wait so we report the child's
# real status after forwarding (not a false green / swallowed crash).
if [ "${TERMINATING:-0}" -eq 1 ]; then
  if kill -0 "$CHILD_PID" 2>/dev/null; then
    wait "$CHILD_PID"
    CHILD_STATUS=$?
  fi
fi
set -e

cleanup_sampler

# Final sample: process may already be gone — still record system/cgroup.
sample_once "$CHILD_PID" "final" || true

diag_line "ts=$(iso_now) label=${LABEL} event=child_exit exit=${CHILD_STATUS} last_signal=${LAST_SIGNAL:-none} elapsed_s=$(elapsed_seconds) budget_s=${BUDGET_SECONDS}"
echo "[fuzz-diag] child_exit=${CHILD_STATUS} last_signal=${LAST_SIGNAL:-none} elapsed_s=$(elapsed_seconds) budget_s=${BUDGET_SECONDS}"

# Preserve exact child status (0, non-zero, or 128+N for signals).
exit "$CHILD_STATUS"
