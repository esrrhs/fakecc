#!/usr/bin/env bash
# Install a host GCC new enough to be a correct difftest oracle.
#
# Ubuntu 24.04 runners ship GCC 13.2: no __builtin_uabs/ulabs, and still
# miscompiles __builtin_mul_overflow_p for tree-optimization/123864.
# Prefer GCC 16 from the Ubuntu toolchain test PPA so the oracle matches
# the GCC 16.2.0 c-torture ports (same as local).
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

sudo apt-get update
sudo apt-get install -y software-properties-common
retry sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install -y gcc-16 g++-16

mkdir -p "$HOME/.local/bin"
ln -sfn "$(command -v gcc-16)" "$HOME/.local/bin/gcc"
ln -sfn "$(command -v g++-16)" "$HOME/.local/bin/g++"

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
