/* Debug-info tests.
 *
 * Two things are being protected here, and the second matters more than it
 * looks.  First, that -g actually describes the program: functions, lines,
 * and variable locations.  Second, that -g is *only* a description — it must
 * never move an instruction.  fakecc used to force scalars onto the stack
 * under -g, which made debug builds ~30% slower and quietly turned -g into
 * -O0; the byte-for-byte .text comparisons below are what keep that from
 * coming back.
 */
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/debug.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Harness                                                             */
/* ------------------------------------------------------------------ */

/* Run the whole pipeline and hand back the EmitModule, so a test can assert
 * on debug records directly instead of re-parsing DWARF. */
static void compile_module(const char *src, int opt_level, int want_debug,
                           EmitModule *out) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "dbgtest.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 0);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, opt_level == 0);
    opt(&ir, opt_level, want_debug);

    emit_module_init(out);
    if (want_debug)
        out->dbg_tu_name = xstrdup("dbgtest.c");
    codegen(&ir, out, want_debug);

    ir_module_free(&ir);
    tu_free(&tu);
    token_array_free(&arr);
}

static void compile_and_link_at(const char *src, const char *path,
                                int opt_level, int want_debug) {
    EmitModule em;
    compile_module(src, opt_level, want_debug, &em);
    EmitModule *mods[1] = { &em };
    emit_link(mods, 1, path, NULL, 0, 0, NULL, 0, want_debug);
    emit_module_free(&em);
}

static void compile_and_link(const char *src, const char *path, int want_debug) {
    compile_and_link_at(src, path, 1, want_debug);
}

static const DebugFunc *find_func(const EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_dbg_funcs; i++)
        if (m->dbg_funcs[i].name && strcmp(m->dbg_funcs[i].name, name) == 0)
            return &m->dbg_funcs[i];
    return NULL;
}

static const DebugVar *find_var(const DebugFunc *f, const char *name) {
    if (!f) return NULL;
    for (size_t i = 0; i < f->num_vars; i++)
        if (f->vars[i].name && strcmp(f->vars[i].name, name) == 0)
            return &f->vars[i];
    return NULL;
}

/* Read one section's bytes out of an ELF file by name, walking e_shoff and
 * the section-header string table.  Returns NULL if the section is absent;
 * the caller frees.  `*out_len` may be 0 for a present-but-empty section. */
static unsigned char *elf_read_section(const char *path, const char *want,
                                       size_t *out_len) {
    *out_len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    unsigned char hdr[64];
    if (fread(hdr, 1, 64, f) != 64) { fclose(f); return NULL; }
    uint64_t shoff = 0;
    for (int i = 0; i < 8; i++) shoff |= (uint64_t)hdr[40 + i] << (8 * i);
    uint16_t shentsize = (uint16_t)(hdr[58] | (hdr[59] << 8));
    uint16_t shnum = (uint16_t)(hdr[60] | (hdr[61] << 8));
    uint16_t shstrndx = (uint16_t)(hdr[62] | (hdr[63] << 8));
    if (shoff == 0 || shnum == 0) { fclose(f); return NULL; }

    fseek(f, (long)(shoff + (size_t)shstrndx * shentsize), SEEK_SET);
    unsigned char sh[64];
    if (fread(sh, 1, 64, f) != 64) { fclose(f); return NULL; }
    uint64_t stroff = 0, strsz = 0;
    for (int i = 0; i < 8; i++) stroff |= (uint64_t)sh[24 + i] << (8 * i);
    for (int i = 0; i < 8; i++) strsz |= (uint64_t)sh[32 + i] << (8 * i);
    char *strtab = malloc((size_t)strsz + 1);
    fseek(f, (long)stroff, SEEK_SET);
    if (fread(strtab, 1, (size_t)strsz, f) != strsz) {
        free(strtab); fclose(f); return NULL;
    }
    strtab[strsz] = '\0';

    unsigned char *data = NULL;
    for (uint16_t s = 0; s < shnum; s++) {
        fseek(f, (long)(shoff + (size_t)s * shentsize), SEEK_SET);
        if (fread(sh, 1, 64, f) != 64) break;
        uint32_t name = (uint32_t)(sh[0] | (sh[1] << 8) | (sh[2] << 16) | (sh[3] << 24));
        if (name >= strsz || strcmp(strtab + name, want) != 0) continue;
        uint64_t off = 0, size = 0;
        for (int i = 0; i < 8; i++) off |= (uint64_t)sh[24 + i] << (8 * i);
        for (int i = 0; i < 8; i++) size |= (uint64_t)sh[32 + i] << (8 * i);
        data = malloc((size_t)size + 1);
        fseek(f, (long)off, SEEK_SET);
        if (size > 0 && fread(data, 1, (size_t)size, f) != size) {
            free(data); data = NULL; break;
        }
        *out_len = (size_t)size;
        break;
    }
    free(strtab);
    fclose(f);
    return data;
}

static int elf_has_section(const char *path, const char *want) {
    size_t len;
    unsigned char *d = elf_read_section(path, want, &len);
    if (!d) return 0;
    free(d);
    return 1;
}

static uint64_t rd_u64(const unsigned char *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)p[i] << (8 * i);
    return v;
}

/* Walk .debug_loc, checking that every list is well formed (a base-address
 * selection entry, then ordered ranges, then a 0/0 terminator) and reporting
 * the highest address any range covers.  A consumer that trips over one
 * malformed list gives up on the whole section, so shape matters. */
static int check_debug_loc(const char *path, uint64_t *out_max_addr,
                          int *out_num_lists) {
    size_t len;
    unsigned char *d = elf_read_section(path, ".debug_loc", &len);
    if (!d) return 0;
    uint64_t max_addr = 0;
    int lists = 0, ok = 1;
    size_t p = 0;
    while (p + 16 <= len) {
        /* Each list opens with a base-address selection entry. */
        if (rd_u64(d + p) != 0xffffffffffffffffULL) { ok = 0; break; }
        uint64_t base = rd_u64(d + p + 8);
        p += 16;
        int entries = 0;
        uint64_t prev_end = 0;
        while (p + 16 <= len) {
            uint64_t begin = rd_u64(d + p), end = rd_u64(d + p + 8);
            p += 16;
            if (begin == 0 && end == 0) break;       /* end of this list */
            if (end <= begin) { ok = 0; break; }
            if (begin < prev_end) { ok = 0; break; }  /* overlapping ranges */
            prev_end = end;
            if (p + 2 > len) { ok = 0; break; }
            uint16_t exprlen = (uint16_t)(d[p] | (d[p + 1] << 8));
            p += 2;
            if (exprlen == 0 || p + exprlen > len) { ok = 0; break; }
            p += exprlen;
            if (base + end > max_addr) max_addr = base + end;
            entries++;
        }
        if (!ok || entries == 0) { ok = 0; break; }
        lists++;
    }
    if (p != len) ok = 0;
    free(d);
    *out_max_addr = max_addr;
    *out_num_lists = lists;
    return ok;
}

static int elf_sym_has(const char *path, const char *symname) {
    /* Use nm if available; otherwise parse .symtab lightly via readelf-like scan. */
    char cmd[512];
    snprintf(cmd, sizeof cmd, "nm '%s' 2>/dev/null | grep -q ' %s$'", path, symname);
    return system(cmd) == 0;
}

/* ------------------------------------------------------------------ */
/* ELF section shape                                                   */
/* ------------------------------------------------------------------ */

static void test_symtab_without_g(void) {
    const char *path = "/tmp/fakecc_test_debug_sym";
    compile_and_link("package main; int main(void) { return 7; }", path, 0);
    T_ASSERT(elf_has_section(path, ".symtab"));
    T_ASSERT(elf_has_section(path, ".strtab"));
    T_ASSERT(!elf_has_section(path, ".debug_info"));
    T_ASSERT(!elf_has_section(path, ".debug_loc"));
    T_ASSERT(elf_sym_has(path, "main"));
    unlink(path);
}

static void test_dwarf_with_g(void) {
    const char *path = "/tmp/fakecc_test_debug_dwarf";
    compile_and_link(
        "package main;\n"
        "int main(void) {\n"
        "    int x;\n"
        "    x = 3;\n"
        "    return x;\n"
        "}\n",
        path, 1);
    T_ASSERT(elf_has_section(path, ".symtab"));
    T_ASSERT(elf_has_section(path, ".debug_info"));
    T_ASSERT(elf_has_section(path, ".debug_line"));
    T_ASSERT(elf_has_section(path, ".debug_abbrev"));
    T_ASSERT(elf_has_section(path, ".debug_frame"));
    T_ASSERT(elf_sym_has(path, "main"));
    unlink(path);
}

static void test_obj_dbg_roundtrip(void) {
    const char *opath = "/tmp/fakecc_test_debug_rt.o";
    const char *epath = "/tmp/fakecc_test_debug_rt";

    EmitModule em;
    compile_module("package main;\n"
                   "int add(int a, int b) { int s; s = a + b; return s * 2; }\n"
                   "int main(void) { int a; a = 1; return add(a, 2); }\n",
                   1, 1, &em);
    emit_obj(&em, opath);
    T_ASSERT(em.num_dbg_funcs > 0);
    /* A promoted local must arrive with a location list, so the roundtrip
     * below is actually exercising range serialization. */
    const DebugVar *s = find_var(find_func(&em, "add"), "s");
    T_ASSERT(s != NULL);
    T_ASSERT(s->num_ranges > 0);
    size_t want_ranges = s->num_ranges;
    emit_module_free(&em);

    EmitModule got;
    T_ASSERT_EQ_INT(emit_obj_read(opath, &got), 0);
    T_ASSERT(got.num_dbg_funcs > 0);
    T_ASSERT(got.num_dbg_lines > 0);
    const DebugVar *s2 = find_var(find_func(&got, "add"), "s");
    T_ASSERT(s2 != NULL);
    T_ASSERT_EQ_INT((int)want_ranges, (int)s2->num_ranges);

    EmitModule *mods[1] = { &got };
    emit_link(mods, 1, epath, NULL, 0, 0, NULL, 0, 1);
    T_ASSERT(elf_has_section(epath, ".debug_line"));
    T_ASSERT(elf_has_section(epath, ".debug_loc"));
    emit_module_free(&got);
    unlink(opath); unlink(epath);
}

/* ------------------------------------------------------------------ */
/* -g must not change the generated code                               */
/* ------------------------------------------------------------------ */

/* A cross-section of language features, so the invariant is checked against
 * loops, calls, floats, aggregates and pointers rather than one toy case. */
static const char *const CODE_SHAPES[] = {
    /* straight-line scalars */
    "package main;\n"
    "int main(void) { int a; int b; a = 3; b = a * 7; return b % 100; }\n",
    /* loop with a φ merge (the case that produces block-entry markers) */
    "package main;\n"
    "int main(void) {\n"
    "    int i; int s;\n"
    "    s = 0; i = 0;\n"
    "    while (i < 10) { if (i % 2 == 0) { s = s + i; } i = i + 1; }\n"
    "    return s;\n"
    "}\n",
    /* recursion + calls: values live across calls, constraining regalloc */
    "package main;\n"
    "int fib(int n) { if (n < 2) { return n; } return fib(n-1) + fib(n-2); }\n"
    "int main(void) { return fib(12) % 100; }\n",
    /* dead store: the marker outlives the value it points at */
    "package main;\n"
    "int main(void) { int used; int dead; dead = 99; used = 4; return used; }\n",
    /* address-taken local: stays in memory, must not gain a location list */
    "package main;\n"
    "int main(void) { int x; int *p; x = 1; p = &x; *p = 41; return x + 1; }\n",
    /* arrays and indexing */
    "package main;\n"
    "int main(void) {\n"
    "    int a[4]; int i; int s;\n"
    "    i = 0; s = 0;\n"
    "    while (i < 4) { a[i] = i * i; i = i + 1; }\n"
    "    i = 0;\n"
    "    while (i < 4) { s = s + a[i]; i = i + 1; }\n"
    "    return s;\n"
    "}\n",
    /* struct members through a pointer */
    "package main;\n"
    "struct P { int x; int y; };\n"
    "int main(void) { struct P p; struct P *q; p.x = 8; p.y = 34; q = &p;\n"
    "                 return q->x + q->y; }\n",
    /* floats: locations come from the XMM allocator, not the GP one */
    "package main;\n"
    "int main(void) { double d; float f; d = 2.5; f = 4.0; return (int)(d * f); }\n",
    /* switch / goto: irregular control flow */
    "package main;\n"
    "int main(void) {\n"
    "    int k; int r;\n"
    "    k = 2; r = 0;\n"
    "    switch (k) { case 1: r = 10; break; case 2: r = 20; default: r = r + 1; }\n"
    "    return r;\n"
    "}\n",
    /* many live values at once, forcing spills */
    "package main;\n"
    "int main(void) {\n"
    "    int a; int b; int c; int d; int e; int f; int g; int h;\n"
    "    a = 1; b = 2; c = 3; d = 4; e = 5; f = 6; g = 7; h = 8;\n"
    "    return (a+b+c+d+e+f+g+h) * (a+h) % 100;\n"
    "}\n",
};
static const size_t NUM_CODE_SHAPES =
    sizeof(CODE_SHAPES) / sizeof(CODE_SHAPES[0]);

/* -g may add DWARF but never move an instruction.  This is what keeps debug
 * builds as fast as release ones, and it must hold at every -O level. */
static void test_g_does_not_change_code(void) {
    for (size_t i = 0; i < NUM_CODE_SHAPES; i++) {
        for (int lvl = 0; lvl <= 1; lvl++) {
            EmitModule plain, dbg;
            compile_module(CODE_SHAPES[i], lvl, 0, &plain);
            compile_module(CODE_SHAPES[i], lvl, 1, &dbg);
            T_ASSERT_EQ_INT((int)plain.text.len, (int)dbg.text.len);
            T_ASSERT_EQ_INT(0, memcmp(plain.text.data, dbg.text.data,
                                      plain.text.len));
            /* Data and read-only sections must match too: a stray debug-only
             * global or literal would also be a behaviour change. */
            T_ASSERT_EQ_INT((int)plain.rodata.len, (int)dbg.rodata.len);
            T_ASSERT_EQ_INT((int)plain.data.len, (int)dbg.data.len);
            emit_module_free(&plain);
            emit_module_free(&dbg);
        }
    }
}

/* Without -g there must be no debug records at all — not merely no sections. */
static void test_no_records_without_g(void) {
    for (int lvl = 0; lvl <= 1; lvl++) {
        EmitModule em;
        compile_module(CODE_SHAPES[1], lvl, 0, &em);
        T_ASSERT_EQ_INT(0, (int)em.num_dbg_funcs);
        T_ASSERT_EQ_INT(0, (int)em.num_dbg_lines);
        T_ASSERT_EQ_INT(0, (int)em.num_dbg_globals);
        T_ASSERT(em.dbg_tu_name == NULL);
        emit_module_free(&em);
    }
}

/* ------------------------------------------------------------------ */
/* Debug record content                                               */
/* ------------------------------------------------------------------ */

/* Every function must have a sane PC range with the prologue end inside it,
 * and every line entry must fall within some function's code. */
static void test_func_and_line_ranges(void) {
    for (int lvl = 0; lvl <= 1; lvl++) {
        EmitModule em;
        compile_module(CODE_SHAPES[2], lvl, 1, &em);

        T_ASSERT(find_func(&em, "fib") != NULL);
        T_ASSERT(find_func(&em, "main") != NULL);
        for (size_t i = 0; i < em.num_dbg_funcs; i++) {
            const DebugFunc *f = &em.dbg_funcs[i];
            T_ASSERT(f->end_pc > f->start_pc);
            T_ASSERT(f->prologue_end_pc >= f->start_pc);
            T_ASSERT(f->prologue_end_pc <= f->end_pc);
            T_ASSERT(f->end_pc <= em.text.len);
        }

        T_ASSERT(em.num_dbg_lines > 0);
        for (size_t i = 0; i < em.num_dbg_lines; i++) {
            const DebugLineEntry *e = &em.dbg_lines[i];
            T_ASSERT(e->file != NULL);
            T_ASSERT(e->line > 0);
            T_ASSERT(e->pc_off <= em.text.len);
        }
        /* The line table drives `break file:N` and stepping, so it has to be
         * laid out in increasing code order. */
        for (size_t i = 1; i < em.num_dbg_lines; i++)
            T_ASSERT(em.dbg_lines[i].pc_off >= em.dbg_lines[i - 1].pc_off);
        emit_module_free(&em);
    }
}

/* A promoted scalar has no fixed home, so it must be described by a location
 * list whose ranges are ordered, non-overlapping, inside the function, and
 * each naming a real register or stack slot. */
static void test_promoted_local_has_loclist(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "int add(int a, int b) { int s; s = a + b; s = s * 2; return s; }\n"
                   "int main(void) { return add(40, 2) % 100; }\n",
                   1, 1, &em);
    const DebugFunc *f = find_func(&em, "add");
    T_ASSERT(f != NULL);

    const DebugVar *s = find_var(f, "s");
    T_ASSERT(s != NULL);
    T_ASSERT_EQ_INT(DBG_VAR_LOCAL, (int)s->kind);
    T_ASSERT(s->num_ranges > 0);
    for (size_t i = 0; i < s->num_ranges; i++) {
        const DebugLocRange *r = &s->ranges[i];
        T_ASSERT(r->pc_end > r->pc_start);
        T_ASSERT(r->pc_start >= f->start_pc);
        T_ASSERT(r->pc_end <= f->end_pc);
        T_ASSERT(r->loc_kind == DBG_LOC_REG || r->loc_kind == DBG_LOC_FBREG);
        if (r->loc_kind == DBG_LOC_REG) T_ASSERT(r->dwarf_reg >= 0);
        if (i > 0) T_ASSERT(r->pc_start >= s->ranges[i - 1].pc_end);
    }

    /* A parameter arrives in its ABI register, so its first range must start
     * at the function's entry rather than after the prologue. */
    const DebugVar *a = find_var(f, "a");
    T_ASSERT(a != NULL);
    T_ASSERT_EQ_INT(DBG_VAR_PARAM, (int)a->kind);
    T_ASSERT(a->num_ranges > 0);
    T_ASSERT_EQ_INT((int)f->start_pc, (int)a->ranges[0].pc_start);
    T_ASSERT_EQ_INT(DBG_LOC_REG, (int)a->ranges[0].loc_kind);
    emit_module_free(&em);
}

/* At -O0 nothing is promoted, so every local is a plain stack slot and no
 * location lists are needed. */
static void test_o0_locals_are_stack_slots(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "int add(int a, int b) { int s; s = a + b; s = s * 2; return s; }\n"
                   "int main(void) { return add(40, 2) % 100; }\n",
                   0, 1, &em);
    const DebugFunc *f = find_func(&em, "add");
    T_ASSERT(f != NULL);
    T_ASSERT(f->num_vars >= 3);
    for (size_t i = 0; i < f->num_vars; i++) {
        const DebugVar *v = &f->vars[i];
        T_ASSERT_EQ_INT(DBG_LOC_FBREG, (int)v->loc_kind);
        T_ASSERT_EQ_INT(0, (int)v->num_ranges);
        T_ASSERT(v->rbp_offset < 0);
    }
    emit_module_free(&em);
}

/* An address-taken local must stay in memory: one stack slot for its whole
 * lifetime, never a location list (gdb has to be able to write through it). */
static void test_address_taken_stays_in_memory(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "int main(void) { int x; int *p; x = 1; p = &x; *p = 41;\n"
                   "                 return x + 1; }\n",
                   1, 1, &em);
    const DebugVar *x = find_var(find_func(&em, "main"), "x");
    T_ASSERT(x != NULL);
    T_ASSERT_EQ_INT(DBG_LOC_FBREG, (int)x->loc_kind);
    T_ASSERT_EQ_INT(0, (int)x->num_ranges);
    T_ASSERT(x->rbp_offset < 0);
    emit_module_free(&em);
}

/* Arrays live in memory and carry their element count, so gdb can print the
 * whole object. */
static void test_array_local_shape(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "int main(void) { int a[4]; a[0] = 1; a[3] = 2; return a[0] + a[3]; }\n",
                   1, 1, &em);
    const DebugVar *a = find_var(find_func(&em, "main"), "a");
    T_ASSERT(a != NULL);
    T_ASSERT_EQ_INT(DBG_TY_ARRAY, (int)a->type_tag);
    T_ASSERT_EQ_INT(4, a->array_len);
    T_ASSERT_EQ_INT(DBG_LOC_FBREG, (int)a->loc_kind);
    T_ASSERT_EQ_INT(0, (int)a->num_ranges);
    emit_module_free(&em);
}

/* A variable whose value the optimizer deleted has no location: reporting a
 * stale slot would make gdb print garbage, so it must report nothing. */
static void test_dead_variable_has_no_location(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "int main(void) { int used; int dead; dead = 99; used = 4;\n"
                   "                 return used; }\n",
                   1, 1, &em);
    const DebugFunc *f = find_func(&em, "main");
    const DebugVar *dead = find_var(f, "dead");
    T_ASSERT(dead != NULL);
    T_ASSERT_EQ_INT(0, (int)dead->num_ranges);
    T_ASSERT_EQ_INT(DBG_LOC_NONE, (int)dead->loc_kind);

    const DebugVar *used = find_var(f, "used");
    T_ASSERT(used != NULL);
    T_ASSERT(used->num_ranges > 0);
    emit_module_free(&em);
}

/* Globals are described by address, resolved from the symbol table at link
 * time, and typed well enough to print. */
static void test_global_var_records(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "int counter = 7;\n"
                   "int *ptr;\n"
                   "int main(void) { counter = counter + 1; return counter; }\n",
                   1, 1, &em);
    int found_counter = 0;
    for (size_t i = 0; i < em.num_dbg_globals; i++) {
        const DebugVar *g = &em.dbg_globals[i];
        if (g->name && strcmp(g->name, "counter") == 0) {
            found_counter = 1;
            T_ASSERT_EQ_INT(DBG_VAR_GLOBAL, (int)g->kind);
            T_ASSERT_EQ_INT(DBG_LOC_ADDR, (int)g->loc_kind);
            T_ASSERT_EQ_INT(4, g->width);
        }
    }
    T_ASSERT(found_counter);
    emit_module_free(&em);
}

/* Float locals are allocated out of the XMM file; their DWARF register
 * numbers must come from that file (xmm0 == DWARF 17), not the GP one. */
static void test_float_var_locations(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "double scale(double d) { double r; r = d * 2.0; return r; }\n"
                   "int main(void) { return (int)scale(21.0); }\n",
                   1, 1, &em);
    const DebugFunc *f = find_func(&em, "scale");
    T_ASSERT(f != NULL);
    const DebugVar *d = find_var(f, "d");
    T_ASSERT(d != NULL);
    T_ASSERT_EQ_INT(DBG_TY_FLOAT, (int)d->type_tag);
    T_ASSERT_EQ_INT(8, d->width);
    if (d->loc_kind == DBG_LOC_REG)
        T_ASSERT(d->dwarf_reg >= 17);
    else
        T_ASSERT_EQ_INT(DBG_LOC_FBREG, (int)d->loc_kind);
    emit_module_free(&em);
}

/* Types must survive into the records, or gdb prints the right bytes with the
 * wrong interpretation. */
static void test_var_type_tags(void) {
    EmitModule em;
    compile_module("package main;\n"
                   "struct P { int x; };\n"
                   "int main(void) {\n"
                   "    int i; unsigned int u; char c; long l; int *p; struct P s;\n"
                   "    i = -1; u = 3; c = 65; l = 5; s.x = 1; p = &i;\n"
                   "    return (int)(i + (int)u + c + (int)l + s.x + *p);\n"
                   "}\n",
                   1, 1, &em);
    const DebugFunc *f = find_func(&em, "main");
    T_ASSERT(f != NULL);

    const DebugVar *v = find_var(f, "u");
    T_ASSERT(v != NULL);
    T_ASSERT_EQ_INT(1, v->is_unsigned);
    T_ASSERT_EQ_INT(4, v->width);

    v = find_var(f, "c");
    T_ASSERT(v != NULL);
    T_ASSERT_EQ_INT(1, v->width);

    v = find_var(f, "l");
    T_ASSERT(v != NULL);
    T_ASSERT_EQ_INT(8, v->width);

    v = find_var(f, "p");
    T_ASSERT(v != NULL);
    T_ASSERT_EQ_INT(DBG_TY_PTR, (int)v->type_tag);

    v = find_var(f, "s");
    T_ASSERT(v != NULL);
    T_ASSERT_EQ_INT(DBG_TY_STRUCT, (int)v->type_tag);
    emit_module_free(&em);
}

/* Every parameter and local named in the source must show up, at both -O
 * levels: a missing DIE is a silently unprintable variable. */
static void test_all_vars_present(void) {
    static const char *const names[] = { "a", "b", "sum", "prod", "i" };
    for (int lvl = 0; lvl <= 1; lvl++) {
        EmitModule em;
        compile_module("package main;\n"
                       "int work(int a, int b) {\n"
                       "    int sum; int prod; int i;\n"
                       "    sum = 0; prod = 1; i = 0;\n"
                       "    while (i < b) { sum = sum + a; prod = prod * 2; i = i + 1; }\n"
                       "    return sum + prod;\n"
                       "}\n"
                       "int main(void) { return work(3, 4) % 100; }\n",
                       lvl, 1, &em);
        const DebugFunc *f = find_func(&em, "work");
        T_ASSERT(f != NULL);
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
            T_ASSERT(find_var(f, names[i]) != NULL);
        emit_module_free(&em);
    }
}

/* Linking must rebase location ranges from per-module .text offsets onto the
 * combined image, or every range in the second module points at the wrong
 * code. */
static void test_link_rebases_loclists(void) {
    EmitModule a, b;
    compile_module("package main;\n"
                   "int helper(int v) { int t; t = v * 3; return t; }\n",
                   1, 1, &a);
    compile_module("package main;\n"
                   "int helper(int v);\n"
                   "int main(void) { int m; m = helper(4); return m % 100; }\n",
                   1, 1, &b);
    /* Second module's ranges start at its own offset 0 before linking. */
    const DebugVar *m_before = find_var(find_func(&b, "main"), "m");
    T_ASSERT(m_before != NULL);
    T_ASSERT(m_before->num_ranges > 0);

    T_ASSERT(m_before->ranges[0].pc_start < a.text.len);

    const char *path = "/tmp/fakecc_test_debug_rebase";
    EmitModule *mods[2] = { &a, &b };
    emit_link(mods, 2, path, NULL, 0, 0, NULL, 0, 1);

    /* After linking, `main` sits past `helper`, so the section must contain a
     * range reaching beyond the first module's code — which only happens if
     * the ranges were rebased onto the combined image. */
    uint64_t max_addr = 0;
    int lists = 0;
    T_ASSERT(check_debug_loc(path, &max_addr, &lists));
    T_ASSERT(lists >= 2);
    T_ASSERT(max_addr > a.text.len);
    unlink(path);
    emit_module_free(&a);
    emit_module_free(&b);
}

static void test_loclist_section_present(void) {
    const char *path = "/tmp/fakecc_test_debug_loclist";
    compile_and_link_at(
        "package main;\n"
        "int add(int a, int b) { int s; s = a + b; s = s * 2; return s; }\n"
        "int main(void) { return add(40, 2) % 100; }\n",
        path, 1, 1);
    uint64_t max_addr = 0;
    int lists = 0;
    T_ASSERT(check_debug_loc(path, &max_addr, &lists));
    T_ASSERT(lists >= 3);   /* a, b and s all move between homes */
    unlink(path);
}

int main(void) {
    test_symtab_without_g();
    test_dwarf_with_g();
    test_obj_dbg_roundtrip();
    test_g_does_not_change_code();
    test_no_records_without_g();
    test_func_and_line_ranges();
    test_promoted_local_has_loclist();
    test_o0_locals_are_stack_slots();
    test_address_taken_stays_in_memory();
    test_array_local_shape();
    test_dead_variable_has_no_location();
    test_global_var_records();
    test_float_var_locations();
    test_var_type_tags();
    test_all_vars_present();
    test_link_rebases_loclists();
    test_loclist_section_present();
    printf("ok\n");
    return 0;
}
