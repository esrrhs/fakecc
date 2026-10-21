#!/usr/bin/env bash
# Install a host GCC new enough to be a correct difftest oracle.
#
# Ubuntu 24.04 runners ship GCC 13.2: no __builtin_uabs/ulabs, and still
# miscompiles __builtin_mul_overflow_p for tree-optimization/123864.
# Prefer GCC 16 from the Ubuntu toolchain test PPA so the oracle matches
# the GCC 16.2.0 c-torture ports (same as local).
#
# On macOS, install gcc via Homebrew (the version varies but is modern
# enough to be a correct oracle for our difftest suite).
set -euo pipefail

# Retry helper: run a command up to N times with a sleep between attempts.
# Launchpad PPA endpoints occasionally return HTTP 504, causing add-apt-repository
# to fail.  Retrying handles these transient network errors.
retry() {
    local attempts=5 delay=30 n=1
    until "$@"; do
        if [ $n -ge $attempts ]; then
            echo "Command failed after $n attempts: $*" >&2
            return 1
        fi
        echo "Attempt $n/$attempts failed for: $*  (retrying in ${delay}s)" >&2
        sleep $delay
        n=$((n + 1))
    done
}

if [ "$(uname -s)" = "Darwin" ]; then
    # macOS: install gcc via Homebrew. `brew install gcc` gives us gcc-N
    # (currently gcc-15 on arm64 macos-latest).  Symlink the versioned names
    # to plain `gcc`/`g++`/`gcov` so test scripts that call `gcc` work.
    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew not found on macOS runner" >&2
        exit 1
    fi
    brew install gcc
    gcc_ver=$(brew list --versions gcc | awk '{print $2}')
    major=$(echo "$gcc_ver" | cut -d. -f1)
    mkdir -p "$HOME/.local/bin"
    ln -sfn "$(brew --prefix)/bin/gcc-$major" "$HOME/.local/bin/gcc"
    ln -sfn "$(brew --prefix)/bin/g++-$major" "$HOME/.local/bin/g++"
    if command -v "$(brew --prefix)/bin/gcov-$major" >/dev/null 2>&1; then
        ln -sfn "$(brew --prefix)/bin/gcov-$major" "$HOME/.local/bin/gcov"
    fi
    if [ -n "${GITHUB_PATH:-}" ]; then
        echo "$HOME/.local/bin" >> "$GITHUB_PATH"
    fi
    export PATH="$HOME/.local/bin:$PATH"
    gcc --version
    echo "macOS: host gcc oracle installed via Homebrew"
    exit 0
fi

sudo apt-get update
sudo apt-get install -y software-properties-common
retry sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
# Launchpad often answers 503, and a Translation-en failure makes apt discard
# a Packages file that already listed gcc-16.  Try briefly, then fall back
# to the conda-forge GCC 16.2.0 build so a dead PPA cannot burn the job.
wait_for_gcc16() {
    local attempts=4 delay=10 n=1
    while true; do
        sudo apt-get update \
            -o Acquire::Retries=2 \
            -o Acquire::Languages=none \
            -o Acquire::https::Timeout=20 \
            -o Acquire::http::Timeout=20 \
            || true
        if apt-cache show gcc-16 >/dev/null 2>&1; then
            return 0
        fi
        if [ "$n" -ge "$attempts" ]; then
            echo "gcc-16 still missing from the toolchain PPA after $n updates" >&2
            return 1
        fi
        echo "gcc-16 not listed yet (attempt $n/$attempts); retrying in ${delay}s" >&2
        sleep "$delay"
        n=$((n + 1))
    done
}
install_conda_gcc16() {
    echo "Installing GCC 16.2.0 from conda-forge" >&2
    mkdir -p "$HOME/.local/bin"
    curl -fsSL --retry 5 --retry-delay 5 --retry-all-errors \
        -o "$HOME/.local/bin/micromamba" \
        https://github.com/mamba-org/micromamba-releases/releases/latest/download/micromamba-linux-64
    chmod +x "$HOME/.local/bin/micromamba"
    "$HOME/.local/bin/micromamba" create -y -p "$HOME/.local/gcc16" \
        -c conda-forge gcc=16.2.0 gxx=16.2.0
    ln -sfn "$HOME/.local/gcc16/bin/gcc" "$HOME/.local/bin/gcc"
    ln -sfn "$HOME/.local/gcc16/bin/g++" "$HOME/.local/bin/g++"
    if [ -x "$HOME/.local/gcc16/bin/gcov" ]; then
        ln -sfn "$HOME/.local/gcc16/bin/gcov" "$HOME/.local/bin/gcov"
    fi
    local libdir="$HOME/.local/gcc16/lib"
    if [ -n "${GITHUB_ENV:-}" ]; then
        echo "LD_LIBRARY_PATH=${libdir}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" >> "$GITHUB_ENV"
    fi
    export LD_LIBRARY_PATH="${libdir}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
}
mkdir -p "$HOME/.local/bin"
ppa_ok=0
if wait_for_gcc16; then
    # One attempt.  apt's own retries already sit on HTTP 503 for several
    # minutes; repeating that five times blows the job before the fallback.
    if sudo apt-get install -y \
        -o Acquire::Retries=2 \
        -o Acquire::Languages=none \
        -o Acquire::https::Timeout=30 \
        gcc-16 g++-16; then
        ppa_ok=1
    fi
fi
if [ "$ppa_ok" -eq 1 ]; then
    ln -sfn "$(command -v gcc-16)" "$HOME/.local/bin/gcc"
    ln -sfn "$(command -v g++-16)" "$HOME/.local/bin/g++"
    if command -v gcov-16 >/dev/null 2>&1; then
        ln -sfn "$(command -v gcov-16)" "$HOME/.local/bin/gcov"
    fi
else
    install_conda_gcc16
fi

if [ -n "${GITHUB_PATH:-}" ]; then
    echo "$HOME/.local/bin" >> "$GITHUB_PATH"
fi
export PATH="$HOME/.local/bin:$PATH"

gcc --version
# Smoke-check the oracle against the known-bad GCC 13 pattern before suites run.
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/pr123864.c" <<'EOF'
[[gnu::noipa]] static int
foo (long long x)
{
  return __builtin_mul_overflow_p (x, ~0U, x);
}
int
main (void)
{
  if (foo (0))
    __builtin_abort ();
  if (foo (0x7fffffff + 1LL))
    __builtin_abort ();
  if (!foo (0x7fffffff + 2LL))
    __builtin_abort ();
  if (foo (-0x7fffffff - 1LL))
    __builtin_abort ();
  if (!foo (-0x7fffffff - 2LL))
    __builtin_abort ();
  return 0;
}
EOF
gcc -std=gnu99 -O0 -o "$tmp/t" "$tmp/pr123864.c"
"$tmp/t"
gcc -std=gnu99 -O2 -o "$tmp/t" "$tmp/pr123864.c"
"$tmp/t"
echo "host gcc oracle: pr123864 OK"
