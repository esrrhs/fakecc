# Gas-subset `.S` assembly (fakeos Should S3)

`fakecc -c foo.S` (also `.s`, `.fakecc.S`) assembles a **gas-compatible AT&T
x86-64 subset** into an ELF `ET_REL` object. Encoding reuses the M1 inline
assembler (`iasm`).

## Directives

| Directive | Support |
|-----------|---------|
| `.text` / `.data` / `.bss` / `.section …` | Accepted (code emitted to `.text`) |
| `.globl` / `.global` | Export label as `STB_GLOBAL` `STT_FUNC` |
| `.extern` | `SHN_UNDEF` |
| `.align` / `.p2align` / `.type` / `.size` | Ignored |
| `.macro name args…` / `.endm` | Expanded; `\arg` and `\()` |
| `.rept N` / `.endr` | Repeat body N times (stub tables) |
| `.incbin "path"` | Embed file bytes in `.rodata` (S5) |
| `#` `//` `;` comments | Stripped |

## Instructions (non-exhaustive)

`mov`/`movb`/`movw`/`movl`/`movq`, `lea`, `push`/`pop`, `add`/`sub`/`xor`/
`and`/`or`/`cmp`/`test`/`xchg`, `jmp`/`call`/`jcc` (incl. indirect `call *%reg` /
`jmp *%reg`), `ret`/`retq`,
`in`/`inb`/`inw`/`inl`, `out`/`outb`/`outw`/`outl`,
`cli`/`sti`/`hlt`/`pause`/`nop`/`ud2`/`syscall`/`sysretq`/`iretq`,
`pushfq`/`popfq`/`pushf`/`popf`,
`lgdt`/`lidt`/`ltr`/`rdmsr`/`wrmsr`/`invlpg`,
`movdqa` `%xmmN` ↔ `disp(%reg)` (and `%xmm`↔`%xmm`) for sysentry/desc XMM
save/restore,
RIP-relative `sym(%rip)` for `lea`/`mov`.

Local labels and forward `jmp`/`call` work within one file; externals get
`R_X86_64_PC32` / `PLT32`.

## Priority migrate (fakeos)

1. **io / usys / crt0** — see `docs/examples/io.S`, `usys.S`, `crt0.S`
2. **switch.asm** — **example-ready**: `docs/examples/switch.S` (`fakecc -c`)
3. **sysentry / desc** — need `movdqa` (now in iasm); see
   `docs/examples/sysentry_xmm.S`. Full ports still want `.data`/`.quad`
   conduit symbols, ISR `.macro`/`.rept` tables, and remaining privileged ops
   (lgdt/lidt/ltr/rdmsr/wrmsr/invlpg already claimed).
4. boot.asm (32-bit — out of scope for this subset)

## C + M1/M2 alternative (no `.S`)

When a stub is a thin SysV wrapper, prefer **naked + GNU inline asm**
(already on this branch):

```c
package main;
__attribute__((naked)) void cpu_halt(void) {
    __asm__ volatile("hlt; ret");
}
__attribute__((naked)) unsigned char inb(unsigned short port) {
    __asm__ volatile(
        "mov %di, %dx\n"
        "xor %eax, %eax\n"
        "in %dx, %al\n"
        "ret");
}
```

Use `.S` when you need file-level `.macro` / `.rept` (IRQ stubs) or
RIP-relative data (`syscall_user_rsp(%rip)`).

## CLI

```text
fakecc -c arch/x86_64/io.S -o io.o
fakecc -c arch/x86_64/switch.S -o switch.o
fakecc --kernel-multiboot -nostdlib boot.o io.o … -o fakeos.elf
```

See also [embed-incbin.md](embed-incbin.md) (S5) and [env-and-flags.md](env-and-flags.md) (N1/N2).

## Related: `--target=` (Should S4)

Unrecognized `--target=` triples print a warning and fall back to the host
default (instead of exiting), so Makefile / `/etc/fakecc.flags` probing stays
safe on single-backend builds. Explicit `--target=x86_64-linux` always selects
the Linux x86-64 ELF backend.
