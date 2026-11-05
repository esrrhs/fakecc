# fakeos remaining gaps (after toolchain M1/M2)

Actionable upstream feedback from **fakeos** @ `573e538` against tip
**fakecc** @ `ce9d7d80` (Merge PR #111: M1–M4 / S1–S5 / N1–N3 surface).

This is **not** a re-landing of the deleted migrate-note set
(`docs: move fakeos toolchain notes out of fakecc`). It only lists what still
blocks a fakecc-only host build of fakeos.

## Status

Tip closes most of the requested **API surface**:

| Item | Tip status |
|------|------------|
| M1 inline asm / M2 naked | done |
| M4 ET_REL + `-mcmodel=kernel` | done |
| S4 `--target=` / S5 `.incbin` | done |
| N1–N3 bootstrap / flags / `-g` | done (surface) |
| S1 `-T` / S2 `--kernel-multiboot` | backend present; **capacity fails on real fakeos** |
| S3 `fakecc -c *.S` | works for many ops; **`%gs:disp` / `%fs:disp` broken** |
| M3 `-O1` va_arg | compiles; **unproven on QEMU** |

Host fakeos still needs **nasm + GNU ld + objcopy** for a full image today.

## Blockers (with evidence)

### 1. `%gs:disp` / `%fs:disp` misassembly (iasm / as_s)

Real fakeos `arch/x86_64/sysentry.asm` uses per-CPU GS:

```asm
; nasm (Intel): [gs:0] = kstack top, [gs:8] = user-RSP scratch
mov     [gs:8], rsp
mov     rsp, [gs:0]
```

Gas AT&T equivalent:

```asm
movq %rsp, %gs:8
movq %gs:0, %rsp
```

**Observed on tip `ce9d7d80`** (`fakecc -c`):

```asm
.text
.globl t
t:
    movq %gs:8, %rsp     /* silently dropped */
    movq %rax, %gs:8     /* becomes mov %rax,%gs  (8E /r) */
    ret
```

```text
# movq %gs:8, %rsp  alone → only `ret` in .text
# movq %rax, %gs:8     →  8e e8   mov %eax,%gs
```

Root cause: `parse_operand` in `src/iasm.c` accepts `%gs` / `%fs` as bare
segment registers (kind 7) for `mov %reg,%sreg` / `mov %sreg,%reg`, but has
**no** memory form `%gs:disp` / `%fs:disp` (absolute disp32 with segment
override). Leftover `:disp` either fails the instruction (load form) or is
ignored after a successful sreg parse (store form → wrong `0x8E` encoding).

**Expected** (GNU `as --64`):

```text
movq %gs:0x8, %rsp
# 65 48 8b 24 25 08 00 00 00
#  ^  GS override + REX.W + MOV r64,r/m64 + SIB abs disp32
```

Codegen already has a hand-rolled Win TLS path (`emit_mov_gs58_reg` in
`src/codegen.c`: `0x65` + `movq %gs:0x58, %reg`). The **assembler** path needs
the same segment-override memory operand for both `%gs:disp` and `%fs:disp`
(loads and stores; at least `mov`/`movq`).

Until fixed, fakeos **must not** swap real `sysentry` onto fakecc `.S` /
inline asm. Former migrate examples worked around this with RIP-relative
`syscall_user_rsp`, which is not the per-CPU GS model fakeos runs.

Minimal failing snippet for a regression:

```asm
.text
.globl gs_roundtrip
gs_roundtrip:
    movq %rdi, %gs:8
    movq %gs:8, %rax
    ret
```

Expect bytes matching `as --64` (GS override `0x65`, not `0x8E` sreg mov).

### 2. `link_kernel` capacity (`OBJ_ARENA` / `objs[]`)

`src/link_kernel.c`:

```c
OBJ_ARENA = 786432,      /* comment: all inputs total ~400 KiB */
static struct obj objs[40];
```

Real fakeos host kernel link (build tree @ assessment):

- **~46** `ET_REL` inputs under `build/*.o` (already `> objs[40]`)
- **`userblob.o` ~4.2 MiB**, **`m5blob.o` ~8.2 MiB** (`.incbin` of user ELFs +
  m5 stage); total object bytes **≫ 768 KiB** arena

So `-T boot/linker.ld` / `--kernel-multiboot` dies with
`object arena full` (and/or object-slot exhaustion) long before layout.

**User** `-T user/user.ld` already links `hello` @ `0x400000` — keep that.

**Propose:**

1. Raise `objs[]` to **≥ 128** (room for kernel C + asm + blobs + growth).
2. Raise `OBJ_ARENA` enough for full kernel + blobs (**tens of MiB**), **or**
   stream large `.incbin` / blob objects (map/file-backed) so the arena only
   holds metadata + small ET_RELs.
3. Revisit `OUT_CAP` (today 2 MiB) if the Multiboot image grows with blobs.

Acceptance: `fakecc -T boot/linker.ld --kernel-multiboot` on a full fakeos
`build/*.o` set produces a QEMU-bootable ELF32 without GNU ld/objcopy.

### 3. Incomplete `.S` ports vs the fakeos tree

Former fakecc `docs/examples/*.S` were **stubs / example-ready sketches**, then
removed from this repo (`e9dfd96c`). They are not drop-in replacements for:

| fakeos object | Still nasm today |
|---------------|------------------|
| `usys` / `crt0` | user syscall stubs + `_start` |
| `io` (+ `cpu_pause` / `enable_sse`) | port I/O + CPU helpers |
| `switch` | context switch / IRQ save |
| `desc` | GDT/IDT/TSS, ISR stubs 0..47+255 |
| `sysentry` | blocked on `%gs:disp` (above) |
| `userblob` / `m5blob` | `.incbin` tables (S5 helps; still need full ports) |

Keep **nasm `-f bin`** for `boot.asm` + `ap_tramp` until there is a 32-bit /
real-mode story (explicit non-goal below).

### 4. M3 `-O1` va_arg — still unproven

`-O1` can compile `kprintf`, but fakeos Makefile still forces **`-O0`**. No
QEMU validation that long-lived format pointers across `va_arg` stay correct.
fakeos stays `-O0` until an upstream fix is proven under `make test`.

## What fakeos will do once fixed

1. Makefile knobs: `USE_FAKECC_ASM=1` / `FAKECC_LINK=1`.
2. Assemble `usys` / `io` / `switch` / `desc` / `crt0` / blob `.S` via
   `$(FCC) -c`; port complete sources from the fakeos tree (not stub examples).
3. Port `sysentry` only after `%gs:disp` encodes correctly (or an agreed
   supported per-CPU conduit).
4. Replace host `ld -T boot/linker.ld` + `objcopy -O elf32-i386` with fakecc
   `-T` / `--kernel-multiboot` once arena/objs land.
5. Keep **nasm only** for `boot` + `ap_tramp` (`-f bin`).

## Non-goals this cycle

- Rewriting boot 32→64 (or real-mode) inside fakecc.
- Changing the fakeos package model / restoring `#include` for the kernel.
- Re-adding the full deleted migrate-note + examples tree to fakecc README.

## References

- fakeos assessment: `docs/fakecc-tip-assessment-notes.md` @ fakeos `573e538`
- fakeos requests: `docs/fakecc-support-requests.md` (M*/S*/N* numbering)
- fakecc tip: `ce9d7d808ed12fa2a01cb0b6d06bb98f7b4db48b`
- Related code: `src/iasm.c` (`parse_operand`, sreg mov), `src/codegen.c`
  (`emit_mov_gs58_reg`), `src/link_kernel.c` (`OBJ_ARENA`, `objs[40]`)
