#!/bin/bash
set -u
FAKECC=${FAKECC:-/home/project/fakecc/build/fakecc}
SRCDIR=/home/project/fakecc/src
FAKEINC=/home/project/fakecc/v0/fakeinc
REALINC=/home/project/fakecc/include
OUT=/home/project/fakecc/v0
FILES="ast cfg codegen common domtree emit ir lexer link main mem2reg opt parser regalloc scalar_opt sema"
read -r -d '' PREAMBLE <<'PKG'
package main;
static int __fakecc_ctzll(unsigned long _v){int c;for(c=0;!(_v&1);c++)_v>>=1;return c;}
static void __fakecc_va_copy(void *dst, void *src){
    char *d = (char*)dst; char *s = (char*)src;
    for(int i = 0; i < 24; i++) d[i] = s[i];
}
PKG
ok=0; fail=0
for f in $FILES; do
    outfile="$OUT/$f.c"
    pre=$(gcc -E -P -nostdinc -I "$FAKEINC" -I "$REALINC" "$SRCDIR/$f.c" 2>/tmp/pp_err_$f)
    if [ $? -ne 0 ] || [ -z "$pre" ]; then echo "PREPROCESS FAIL: $f"; cat /tmp/pp_err_$f; fail=$((fail+1)); continue; fi
    pre=$(echo "$pre" | sed 's/__attribute__((__noreturn__))//g; s/__attribute__((.*))//g')
    printf '%s\n' "$PREAMBLE" > "$outfile"
    echo "$pre" >> "$outfile"
    "$FAKECC" "$outfile" -c -o "$OUT/$f.o" 2>/tmp/cc_err_$f
    rc=$?
    if [ $rc -ne 0 ]; then echo "COMPILE FAIL ($rc): $f"; cat /tmp/cc_err_$f; fail=$((fail+1)); else ok=$((ok+1)); fi
done
echo "=== compile result: $ok ok, $fail fail ==="
if [ $fail -ne 0 ]; then exit 1; fi
OBJS=""; for f in $FILES; do OBJS="$OBJS $OUT/$f.o"; done
gcc $OBJS -o "$OUT/bootstrap_fakecc" -lm 2>/tmp/link_err
if [ $? -ne 0 ]; then echo "LINK FAIL"; cat /tmp/link_err; exit 1; fi
echo "=== linked $OUT/bootstrap_fakecc ==="
ls -la "$OUT/bootstrap_fakecc"
