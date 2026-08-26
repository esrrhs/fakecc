#!/usr/bin/env bash
# Install a host GCC new enough to be a correct difftest oracle.
#
# Ubuntu 24.04 runners ship GCC 13.2, which still miscompiles
# __builtin_mul_overflow_p for the mixed signedness case in
# tree-optimization/123864 (gcc exits 134; fakecc matches GCC 16).
# Prefer GCC 16 from the Ubuntu toolchain test PPA.
set -euo pipefail

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
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
# Smoke-check the oracle against the known-bad pattern before suites run.
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
