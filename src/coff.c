/* AMD64 COFF relocatable object writer/reader for x86_64-windows.
 *
 * EmitModule stays ELF-shaped (R_X86_64_* + SECT_*).  This file maps to
 * IMAGE_REL_AMD64_* on the way out and back in, keeps addends in section
 * bytes the COFF way, and round-trips fakecc's GOTPCREL via a private
 * reloc type so pe_link can still build its GOT.
 */

#include "fakecc/pe.h"
#include "fakecc/emit.h"
#include "fakecc/common.h"
#include "fakecc/debug.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COFF_FILE_HDR  20
#define COFF_SECT_HDR  40
#define COFF_SYM_SIZE  18
#define COFF_RELOC_SZ  10

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_TLS     6
#define STB_LOCAL   0
#define STB_GLOBAL  1
#define STB_WEAK    2

/* ------------------------------------------------------------------ */
/* Little helpers                                                      */
/* ------------------------------------------------------------------ */

static void bu8(Buffer *b, uint8_t v)  { buffer_append(b, (const char *)&v, 1); }
static void bu16(Buffer *b, uint16_t v) { buffer_append(b, (const char *)&v, 2); }
static void bu32(Buffer *b, uint32_t v) { buffer_append(b, (const char *)&v, 4); }

static uint16_t rd_u16(const unsigned char *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}
static uint32_t rd_u32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int32_t rd_i32(const unsigned char *p) {
    return (int32_t)rd_u32(p);
}

static uint32_t align_chars(size_t a) {
    unsigned e = 0;
    size_t v = a ? a : 1;
    while ((size_t)1 << e < v && e < 12) e++;
    if (e < 1) e = 1;
    return (uint32_t)(e + 1) << 20; /* IMAGE_SCN_ALIGN_* */
}

static size_t align_up(size_t v, size_t a) {
    if (!a) return v;
    return (v + a - 1) & ~(a - 1);
}

/* ------------------------------------------------------------------ */
/* String table                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    Buffer buf; /* starts with 4-byte size placeholder */
} StrTab;

static void strtab_init(StrTab *s) {
    buffer_init(&s->buf);
    bu32(&s->buf, 0);
}

static uint32_t strtab_add(StrTab *s, const char *name) {
    if (!name) name = "";
    size_t n = strlen(name);
    if (n < 8) return 0; /* caller embeds short names */
    uint32_t off = (uint32_t)s->buf.len;
    buffer_append(&s->buf, name, n + 1);
    return off;
}

static void strtab_finish(StrTab *s) {
    uint32_t sz = (uint32_t)s->buf.len;
    memcpy(s->buf.data, &sz, 4);
}

static void write_sym_name(Buffer *b, const char *name, StrTab *st) {
    char nm[8];
    memset(nm, 0, 8);
    if (!name) name = "";
    size_t n = strlen(name);
    if (n <= 8) {
        memcpy(nm, name, n);
        buffer_append(b, nm, 8);
    } else {
        uint32_t off = strtab_add(st, name);
        memset(nm, 0, 8);
        memcpy(nm + 4, &off, 4);
        buffer_append(b, nm, 8);
    }
}

/* ------------------------------------------------------------------ */
/* Reloc type mapping                                                  */
/* ------------------------------------------------------------------ */

static uint16_t elf_to_coff_reloc(uint32_t t) {
    switch (t) {
    case R_X86_64_64:       return IMAGE_REL_AMD64_ADDR64;
    case R_X86_64_PC32:     return IMAGE_REL_AMD64_REL32;
    case R_X86_64_PLT32:    return IMAGE_REL_AMD64_REL32;
    case R_X86_64_32:       return IMAGE_REL_AMD64_ADDR32;
    case R_X86_64_32S:      return IMAGE_REL_AMD64_ADDR32;
    case R_PE_ADDR32NB:     return IMAGE_REL_AMD64_ADDR32NB;
    case R_PE_SECREL:       return IMAGE_REL_AMD64_SECREL;
    case R_PE_SECTION:      return IMAGE_REL_AMD64_SECTION;
    case R_X86_64_GOTPCREL:
    case R_X86_64_GOTPCRELX:
    case R_X86_64_REX_GOTPCRELX:
        return IMAGE_REL_AMD64_FAKECC_GOTPCREL;
    default:
        return IMAGE_REL_AMD64_REL32;
    }
}

static uint32_t coff_to_elf_reloc(uint16_t t) {
    switch (t) {
    case IMAGE_REL_AMD64_ADDR64:          return R_X86_64_64;
    case IMAGE_REL_AMD64_ADDR32:          return R_X86_64_32;
    case IMAGE_REL_AMD64_ADDR32NB:        return R_PE_ADDR32NB;
    case IMAGE_REL_AMD64_REL32:           return R_X86_64_PC32;
    case IMAGE_REL_AMD64_REL32_1:         return R_X86_64_PC32;
    case IMAGE_REL_AMD64_REL32_2:         return R_X86_64_PC32;
    case IMAGE_REL_AMD64_REL32_3:         return R_X86_64_PC32;
    case IMAGE_REL_AMD64_REL32_4:         return R_X86_64_PC32;
    case IMAGE_REL_AMD64_REL32_5:         return R_X86_64_PC32;
    case IMAGE_REL_AMD64_SECREL:          return R_PE_SECREL;
    case IMAGE_REL_AMD64_SECTION:         return R_PE_SECTION;
    case IMAGE_REL_AMD64_SREL32:          return R_X86_64_PC32;
    case IMAGE_REL_AMD64_FAKECC_GOTPCREL: return R_X86_64_GOTPCREL;
    default:                              return R_X86_64_PC32;
    }
}

static int reloc_width(uint16_t t) {
    switch (t) {
    case IMAGE_REL_AMD64_ADDR64: return 8;
    case IMAGE_REL_AMD64_ADDR32:
    case IMAGE_REL_AMD64_ADDR32NB:
    case IMAGE_REL_AMD64_REL32:
    case IMAGE_REL_AMD64_REL32_1:
    case IMAGE_REL_AMD64_REL32_2:
    case IMAGE_REL_AMD64_REL32_3:
    case IMAGE_REL_AMD64_REL32_4:
    case IMAGE_REL_AMD64_REL32_5:
    case IMAGE_REL_AMD64_SECREL:
    case IMAGE_REL_AMD64_SREL32:
    case IMAGE_REL_AMD64_FAKECC_GOTPCREL:
        return 4;
    case IMAGE_REL_AMD64_SECTION:
        return 2;
    default:
        return 4;
    }
}

static int32_t reloc_extra_addend(uint16_t t) {
    /* REL32_N means the PC is N bytes past the reloc field.  EmitModule
     * stores a single addend as for REL32 (PC = field+4), so fold the
     * extra displacement into the addend when reading. */
    switch (t) {
    case IMAGE_REL_AMD64_REL32_1: return -1;
    case IMAGE_REL_AMD64_REL32_2: return -2;
    case IMAGE_REL_AMD64_REL32_3: return -3;
    case IMAGE_REL_AMD64_REL32_4: return -4;
    case IMAGE_REL_AMD64_REL32_5: return -5;
    default: return 0;
    }
}

/* ------------------------------------------------------------------ */
/* Section plan for the writer                                         */
/* ------------------------------------------------------------------ */

enum {
    WS_TEXT = 0,
    WS_RDATA,
    WS_DATA,
    WS_BSS,
    WS_TDATA,
    WS_TBSS,
    WS_INIT,
    WS_FINI,
    WS_PDATA,
    WS_XDATA,
    WS_DRECTVE,
    WS_DBG,
    WS_COUNT
};

typedef struct {
    const char *name;
    uint32_t chars;
    size_t align;
    const char *data;
    size_t size;       /* VirtualSize / SizeOfRawData for NOBITS = size, raw=0 */
    int nobits;
    int present;
    uint16_t sect_num; /* 1-based COFF section number, 0 if absent */
    uint32_t file_off;
    uint32_t reloc_off;
    uint32_t nreloc;
    uint32_t sym_idx;  /* COFF symbol index of section symbol */
} WSect;

static void patch_addend(char *base, size_t len, size_t off, int32_t addend,
                         int width) {
    if (off + (size_t)width > len) return;
    if (width == 8) {
        int64_t v = addend;
        memcpy(base + off, &v, 8);
    } else if (width == 4) {
        memcpy(base + off, &addend, 4);
    } else if (width == 2) {
        int16_t v = (int16_t)addend;
        memcpy(base + off, &v, 2);
    }
}

/* ------------------------------------------------------------------ */
/* Unwind (.pdata / .xdata) emission                                   */
/* ------------------------------------------------------------------ */

#define UWOP_PUSH_NONVOL 0
#define UWOP_ALLOC_LARGE 1
#define UWOP_ALLOC_SMALL 2
#define UWOP_SET_FPREG   3

static void emit_unwind_blob(const EmitModule *m, Buffer *pdata, Buffer *xdata,
                             Buffer *pdata_rel, uint32_t xdata_sym,
                             const uint32_t *func_sym_of) {
    buffer_init(pdata);
    buffer_init(xdata);
    buffer_init(pdata_rel);
    for (size_t i = 0; i < m->num_win_unwinds; i++) {
        const EmitWinUnwind *u = &m->win_unwinds[i];
        if (u->end <= u->begin) continue;

        /* Find function symbol index among EmitModule symbols for reloc. */
        int fsym = -1;
        for (size_t s = 0; s < m->num_syms; s++) {
            const EmitSymbol *sy = &m->syms[s];
            if (sy->shndx == SECT_TEXT && sy->type == STT_FUNC
                && sy->value == u->begin) {
                fsym = (int)s;
                break;
            }
        }
        if (fsym < 0 || !func_sym_of || func_sym_of[fsym] == (uint32_t)-1)
            continue;

        uint32_t xoff = (uint32_t)xdata->len;
        /* Build UNWIND_INFO codes in reverse execution order. */
        uint16_t codes[32];
        int nc = 0;
        uint8_t prolog = (uint8_t)(u->after_prolog > u->begin
                                   ? u->after_prolog - u->begin : 0);

        if (u->stack_alloc > 0) {
            uint8_t off = prolog;
            if (u->stack_alloc <= 128 && (u->stack_alloc % 8) == 0) {
                uint8_t info = (uint8_t)(u->stack_alloc / 8 - 1);
                codes[nc++] = (uint16_t)(off | ((UWOP_ALLOC_SMALL | (info << 4)) << 8));
            } else if (u->stack_alloc <= 512 * 1024) {
                codes[nc++] = (uint16_t)((u->stack_alloc / 8) & 0xffff);
                codes[nc++] = (uint16_t)(off | (UWOP_ALLOC_LARGE << 8));
            } else {
                codes[nc++] = (uint16_t)((u->stack_alloc >> 16) & 0xffff);
                codes[nc++] = (uint16_t)(u->stack_alloc & 0xffff);
                codes[nc++] = (uint16_t)(off | ((UWOP_ALLOC_LARGE | (1 << 4)) << 8));
            }
        }
        for (int p = 7; p >= 0; p--) {
            if (u->pushed[p] < 0) continue;
            uint8_t off = (uint8_t)(u->after_set_fp > u->begin
                                    ? (u->after_set_fp - u->begin)
                                      + (size_t)(p + 1) * 8
                                    : 0);
            if (u->after_prolog > u->begin && off > prolog) off = prolog;
            codes[nc++] = (uint16_t)(off
                | ((UWOP_PUSH_NONVOL | ((uint8_t)u->pushed[p] << 4)) << 8));
        }
        {
            uint8_t off = (uint8_t)(u->after_set_fp > u->begin
                                    ? u->after_set_fp - u->begin : 0);
            codes[nc++] = (uint16_t)(off | (UWOP_SET_FPREG << 8));
        }
        {
            uint8_t off = (uint8_t)(u->after_push_rbp > u->begin
                                    ? u->after_push_rbp - u->begin : 1);
            codes[nc++] = (uint16_t)(off
                | ((UWOP_PUSH_NONVOL | (5 /* RBP */ << 4)) << 8));
        }

        int ncodes = nc;
        if (ncodes & 1) {
            codes[ncodes++] = 0; /* pad to even */
        }
        bu8(xdata, 1 | (0 << 3)); /* Version=1, Flags=0 */
        bu8(xdata, prolog);
        bu8(xdata, (uint8_t)nc);
        bu8(xdata, (uint8_t)((5 /* RBP */) | (0 << 4))); /* FrameRegister|Offset */
        for (int c = 0; c < ncodes; c++)
            bu16(xdata, codes[c]);

        /* RUNTIME_FUNCTION */
        uint32_t pbase = (uint32_t)pdata->len;
        bu32(pdata, 0); /* BeginAddress */
        bu32(pdata, (uint32_t)(u->end - u->begin)); /* EndAddress addend */
        bu32(pdata, xoff); /* UnwindInfo addend (sect-relative via xdata sym) */

        /* Relocs: ADDR32NB against function for begin/end, xdata for unwind. */
        uint32_t fcoff = func_sym_of[fsym];
        bu32(pdata_rel, pbase + 0); bu32(pdata_rel, fcoff); bu16(pdata_rel, IMAGE_REL_AMD64_ADDR32NB);
        bu32(pdata_rel, pbase + 4); bu32(pdata_rel, fcoff); bu16(pdata_rel, IMAGE_REL_AMD64_ADDR32NB);
        bu32(pdata_rel, pbase + 8); bu32(pdata_rel, xdata_sym); bu16(pdata_rel, IMAGE_REL_AMD64_ADDR32NB);
        (void)ncodes;
    }
}

/* ------------------------------------------------------------------ */
/* Writer                                                              */
/* ------------------------------------------------------------------ */

int coff_write_object(const EmitModule *m, const char *path) {
    if (!m || !path) return -1;

    Buffer text = m->text, rdata = m->rodata, data = m->data;
    Buffer tdata = m->tdata, init = m->init_array, fini = m->fini_array;
    /* Mutable copies so we can poke reloc addends without touching *m. */
    Buffer text_c, rdata_c, data_c, tdata_c, init_c, fini_c;
    buffer_init(&text_c); buffer_init(&rdata_c); buffer_init(&data_c);
    buffer_init(&tdata_c); buffer_init(&init_c); buffer_init(&fini_c);
    buffer_append(&text_c, text.data, text.len);
    buffer_append(&rdata_c, rdata.data, rdata.len);
    buffer_append(&data_c, data.data, data.len);
    buffer_append(&tdata_c, tdata.data, tdata.len);
    buffer_append(&init_c, init.data, init.len);
    buffer_append(&fini_c, fini.data, fini.len);

    /* Apply EmitModule addends into section bytes (COFF inline addends). */
    for (size_t i = 0; i < m->num_relocs; i++) {
        const EmitReloc *r = &m->relocs[i];
        uint16_t ct = elf_to_coff_reloc(r->type);
        patch_addend(text_c.data, text_c.len, r->offset, r->addend,
                     reloc_width(ct));
    }
    for (size_t i = 0; i < m->num_data_relocs; i++) {
        const EmitReloc *r = &m->data_relocs[i];
        uint16_t ct = elf_to_coff_reloc(r->type);
        Buffer *dst = &data_c;
        if (r->shndx == SECT_RODATA) dst = &rdata_c;
        else if (r->shndx == SECT_TDATA) dst = &tdata_c;
        else if (r->shndx == SECT_INIT_ARRAY) dst = &init_c;
        else if (r->shndx == SECT_FINI_ARRAY) dst = &fini_c;
        patch_addend(dst->data, dst->len, r->offset, r->addend,
                     reloc_width(ct));
    }

    Buffer fakecc_dbg;
    buffer_init(&fakecc_dbg);
    int have_dbg = (m->num_dbg_lines > 0 || m->num_dbg_funcs > 0 ||
                    m->num_dbg_globals > 0 || m->dbg_tu_name != NULL);
    if (have_dbg) debug_serialize(m, &fakecc_dbg);

    WSect sec[WS_COUNT];
    memset(sec, 0, sizeof sec);
    sec[WS_TEXT] = (WSect){ ".text",
        IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ
            | align_chars(m->text_align ? m->text_align : 16),
        m->text_align ? m->text_align : 16, text_c.data, text_c.len, 0, 1, 0,0,0,0,0 };
    sec[WS_RDATA] = (WSect){ ".rdata",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
            | align_chars(m->rodata_align ? m->rodata_align : 8),
        m->rodata_align ? m->rodata_align : 8, rdata_c.data, rdata_c.len, 0,
        rdata_c.len > 0, 0,0,0,0,0 };
    sec[WS_DATA] = (WSect){ ".data",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
            | align_chars(m->data_align ? m->data_align : 8),
        m->data_align ? m->data_align : 8, data_c.data, data_c.len, 0,
        data_c.len > 0, 0,0,0,0,0 };
    sec[WS_BSS] = (WSect){ ".bss",
        IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
            | align_chars(m->bss_align ? m->bss_align : 8),
        m->bss_align ? m->bss_align : 8, NULL, m->bss_size, 1,
        m->bss_size > 0, 0,0,0,0,0 };
    sec[WS_TDATA] = (WSect){ ".tls$",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
            | align_chars(m->tdata_align ? m->tdata_align : 8),
        m->tdata_align ? m->tdata_align : 8, tdata_c.data, tdata_c.len, 0,
        tdata_c.len > 0, 0,0,0,0,0 };
    sec[WS_TBSS] = (WSect){ ".tbss",
        IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
            | align_chars(m->tbss_align ? m->tbss_align : 8),
        m->tbss_align ? m->tbss_align : 8, NULL, m->tbss_size, 1,
        m->tbss_size > 0, 0,0,0,0,0 };
    sec[WS_INIT] = (WSect){ ".CRT$XCU",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
            | align_chars(8),
        8, init_c.data, init_c.len, 0, init_c.len > 0, 0,0,0,0,0 };
    sec[WS_FINI] = (WSect){ ".CRT$XTX",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
            | align_chars(8),
        8, fini_c.data, fini_c.len, 0, fini_c.len > 0, 0,0,0,0,0 };
    sec[WS_DRECTVE] = (WSect){ ".drectve",
        IMAGE_SCN_LNK_INFO | IMAGE_SCN_LNK_REMOVE | align_chars(1),
        1, m->drectve.data, m->drectve.len, 0, m->drectve.len > 0, 0,0,0,0,0 };
    sec[WS_DBG] = (WSect){ ".fakecc_dbg",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_LNK_REMOVE
            | align_chars(1),
        1, fakecc_dbg.data, fakecc_dbg.len, 0, have_dbg, 0,0,0,0,0 };

    /* Placeholder pdata/xdata; filled after we know function COFF sym indices. */
    Buffer pdata_b, xdata_b, pdata_rel;
    buffer_init(&pdata_b); buffer_init(&xdata_b); buffer_init(&pdata_rel);
    int want_unwind = m->num_win_unwinds > 0;
    sec[WS_PDATA] = (WSect){ ".pdata",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | align_chars(4),
        4, NULL, 0, 0, want_unwind, 0,0,0,0,0 };
    sec[WS_XDATA] = (WSect){ ".xdata",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | align_chars(4),
        4, NULL, 0, 0, want_unwind, 0,0,0,0,0 };

    uint16_t nsec = 0;
    for (int i = 0; i < WS_COUNT; i++) {
        if (!sec[i].present) continue;
        sec[i].sect_num = ++nsec;
    }

    /* Count relocs per section. */
    for (size_t i = 0; i < m->num_relocs; i++)
        sec[WS_TEXT].nreloc++;
    for (size_t i = 0; i < m->num_data_relocs; i++) {
        const EmitReloc *r = &m->data_relocs[i];
        if (r->shndx == SECT_RODATA) sec[WS_RDATA].nreloc++;
        else if (r->shndx == SECT_TDATA) sec[WS_TDATA].nreloc++;
        else if (r->shndx == SECT_INIT_ARRAY) sec[WS_INIT].nreloc++;
        else if (r->shndx == SECT_FINI_ARRAY) sec[WS_FINI].nreloc++;
        else sec[WS_DATA].nreloc++;
    }

    StrTab st;
    strtab_init(&st);
    /* Reserve long section names in the string table before it is sealed. */
    for (int i = 0; i < WS_COUNT; i++) {
        if (sec[i].present && sec[i].name && strlen(sec[i].name) > 8)
            strtab_add(&st, sec[i].name);
    }

    /* Build symbol table: [section symbols][locals][globals/weaks/undef]. */
    Buffer symtab;
    buffer_init(&symtab);
    uint32_t *emit_to_coff = calloc(m->num_syms ? m->num_syms : 1, sizeof(uint32_t));
    if (!emit_to_coff) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    for (size_t i = 0; i < m->num_syms; i++) emit_to_coff[i] = (uint32_t)-1;

    /* Section symbols (+ aux). */
    for (int i = 0; i < WS_COUNT; i++) {
        if (!sec[i].present) continue;
        sec[i].sym_idx = (uint32_t)(symtab.len / COFF_SYM_SIZE);
        write_sym_name(&symtab, sec[i].name, &st);
        bu32(&symtab, 0);
        bu16(&symtab, sec[i].sect_num);
        bu16(&symtab, 0);
        bu8(&symtab, IMAGE_SYM_CLASS_STATIC);
        bu8(&symtab, 1); /* one aux */
        /* Aux section definition (exactly 18 bytes). */
        bu32(&symtab, (uint32_t)sec[i].size);
        bu16(&symtab, (uint16_t)sec[i].nreloc);
        bu16(&symtab, 0);          /* NumberOfLinenumbers */
        bu32(&symtab, 0);          /* CheckSum */
        bu16(&symtab, 0);          /* Number (COMDAT) */
        bu8(&symtab, 0);           /* Selection */
        bu8(&symtab, 0);           /* bReserved */
        bu16(&symtab, 0);          /* unused */
    }

    /* Pass 1: LOCAL symbols */
    for (size_t i = 0; i < m->num_syms; i++) {
        const EmitSymbol *s = &m->syms[i];
        if (s->binding != STB_LOCAL) continue;
        if (s->type == STT_SECTION) continue; /* we emit our own */
        emit_to_coff[i] = (uint32_t)(symtab.len / COFF_SYM_SIZE);
        write_sym_name(&symtab, s->name ? s->name : "", &st);
        bu32(&symtab, (uint32_t)s->value);
        uint16_t sn = 0;
        if (s->shndx == SECT_TEXT && sec[WS_TEXT].present) sn = sec[WS_TEXT].sect_num;
        else if (s->shndx == SECT_RODATA && sec[WS_RDATA].present) sn = sec[WS_RDATA].sect_num;
        else if (s->shndx == SECT_DATA && sec[WS_DATA].present) sn = sec[WS_DATA].sect_num;
        else if (s->shndx == SECT_BSS && sec[WS_BSS].present) sn = sec[WS_BSS].sect_num;
        else if (s->shndx == SECT_TDATA && sec[WS_TDATA].present) sn = sec[WS_TDATA].sect_num;
        else if (s->shndx == SECT_TBSS && sec[WS_TBSS].present) sn = sec[WS_TBSS].sect_num;
        else if (s->shndx == SECT_INIT_ARRAY && sec[WS_INIT].present) sn = sec[WS_INIT].sect_num;
        else if (s->shndx == SECT_FINI_ARRAY && sec[WS_FINI].present) sn = sec[WS_FINI].sect_num;
        else if (s->shndx == SHN_COMMON) sn = IMAGE_SYM_UNDEFINED;
        bu16(&symtab, sn);
        uint16_t ty = (s->type == STT_FUNC) ? 0x20 : 0;
        bu16(&symtab, ty);
        bu8(&symtab, IMAGE_SYM_CLASS_STATIC);
        bu8(&symtab, 0);
    }

    /* Pass 2: GLOBAL / WEAK / UNDEF / COMMON */
    for (size_t i = 0; i < m->num_syms; i++) {
        const EmitSymbol *s = &m->syms[i];
        if (s->binding == STB_LOCAL) continue;
        if (s->type == STT_SECTION) continue;
        emit_to_coff[i] = (uint32_t)(symtab.len / COFF_SYM_SIZE);
        write_sym_name(&symtab, s->name ? s->name : "", &st);

        if (s->shndx == SHN_COMMON) {
            bu32(&symtab, (uint32_t)(s->size ? s->size : s->value));
            bu16(&symtab, IMAGE_SYM_UNDEFINED);
            bu16(&symtab, 0);
            bu8(&symtab, IMAGE_SYM_CLASS_EXTERNAL);
            bu8(&symtab, 0);
            continue;
        }
        if (s->shndx == SECT_UNDEF) {
            if (s->binding == STB_WEAK) {
                bu32(&symtab, 0);
                bu16(&symtab, IMAGE_SYM_UNDEFINED);
                bu16(&symtab, 0);
                bu8(&symtab, IMAGE_SYM_CLASS_WEAK_EXTERNAL);
                bu8(&symtab, 1);
                /* Aux weak-external (exactly 18 bytes). */
                bu32(&symtab, 0); /* TagIndex */
                bu32(&symtab, IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY);
                bu32(&symtab, 0);
                bu16(&symtab, 0);
                bu16(&symtab, 0);
                bu16(&symtab, 0);
            } else {
                bu32(&symtab, 0);
                bu16(&symtab, IMAGE_SYM_UNDEFINED);
                bu16(&symtab, (s->type == STT_FUNC) ? 0x20 : 0);
                bu8(&symtab, IMAGE_SYM_CLASS_EXTERNAL);
                bu8(&symtab, 0);
            }
            continue;
        }

        bu32(&symtab, (uint32_t)s->value);
        uint16_t sn = 0;
        if (s->shndx == SECT_TEXT) sn = sec[WS_TEXT].sect_num;
        else if (s->shndx == SECT_RODATA) sn = sec[WS_RDATA].sect_num;
        else if (s->shndx == SECT_DATA) sn = sec[WS_DATA].sect_num;
        else if (s->shndx == SECT_BSS) sn = sec[WS_BSS].sect_num;
        else if (s->shndx == SECT_TDATA) sn = sec[WS_TDATA].sect_num;
        else if (s->shndx == SECT_TBSS) sn = sec[WS_TBSS].sect_num;
        else if (s->shndx == SECT_INIT_ARRAY) sn = sec[WS_INIT].sect_num;
        else if (s->shndx == SECT_FINI_ARRAY) sn = sec[WS_FINI].sect_num;
        bu16(&symtab, sn);
        bu16(&symtab, (s->type == STT_FUNC) ? 0x20 : 0);
        if (s->binding == STB_WEAK) {
            bu8(&symtab, IMAGE_SYM_CLASS_WEAK_EXTERNAL);
            bu8(&symtab, 1);
            bu32(&symtab, 0);
            bu32(&symtab, IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY);
            bu32(&symtab, 0);
            bu16(&symtab, 0);
            bu16(&symtab, 0);
            bu16(&symtab, 0);
        } else {
            bu8(&symtab, IMAGE_SYM_CLASS_EXTERNAL);
            bu8(&symtab, 0);
        }
    }

    /* Build unwind now that function COFF indices are known. */
    if (want_unwind) {
        emit_unwind_blob(m, &pdata_b, &xdata_b, &pdata_rel,
                         sec[WS_XDATA].sym_idx, emit_to_coff);
        sec[WS_PDATA].data = pdata_b.data;
        sec[WS_PDATA].size = pdata_b.len;
        sec[WS_PDATA].nreloc = (uint32_t)(pdata_rel.len / COFF_RELOC_SZ);
        sec[WS_XDATA].data = xdata_b.data;
        sec[WS_XDATA].size = xdata_b.len;
        /* Patch section-symbol aux SizeOfRawData / NumberOfRelocations. */
        if (sec[WS_PDATA].present) {
            uint32_t si = sec[WS_PDATA].sym_idx;
            memcpy(symtab.data + (size_t)si * COFF_SYM_SIZE + COFF_SYM_SIZE,
                   &sec[WS_PDATA].size, 4);
            uint16_t nr = (uint16_t)sec[WS_PDATA].nreloc;
            memcpy(symtab.data + (size_t)si * COFF_SYM_SIZE + COFF_SYM_SIZE + 4,
                   &nr, 2);
        }
        if (sec[WS_XDATA].present) {
            uint32_t si = sec[WS_XDATA].sym_idx;
            memcpy(symtab.data + (size_t)si * COFF_SYM_SIZE + COFF_SYM_SIZE,
                   &sec[WS_XDATA].size, 4);
        }
    }

    /* Layout: headers, then section data, then relocs, then symtab+strtab. */
    size_t hdr_sz = COFF_FILE_HDR + (size_t)nsec * COFF_SECT_HDR;
    size_t cursor = align_up(hdr_sz, 4);
    for (int i = 0; i < WS_COUNT; i++) {
        if (!sec[i].present || sec[i].nobits) continue;
        cursor = align_up(cursor, sec[i].align ? sec[i].align : 1);
        sec[i].file_off = (uint32_t)cursor;
        cursor += sec[i].size;
    }
    for (int i = 0; i < WS_COUNT; i++) {
        if (!sec[i].present || !sec[i].nreloc) continue;
        cursor = align_up(cursor, 4);
        sec[i].reloc_off = (uint32_t)cursor;
        cursor += (size_t)sec[i].nreloc * COFF_RELOC_SZ;
    }
    size_t sym_off = align_up(cursor, 4);
    strtab_finish(&st);
    size_t str_off = sym_off + symtab.len;

    /* Assemble reloc tables. */
    Buffer relocs[WS_COUNT];
    for (int i = 0; i < WS_COUNT; i++) buffer_init(&relocs[i]);

    for (size_t i = 0; i < m->num_relocs; i++) {
        const EmitReloc *r = &m->relocs[i];
        uint32_t si = (r->sym < m->num_syms) ? emit_to_coff[r->sym] : (uint32_t)-1;
        if (si == (uint32_t)-1) continue;
        uint16_t ct = elf_to_coff_reloc(r->type);
        bu32(&relocs[WS_TEXT], (uint32_t)r->offset);
        bu32(&relocs[WS_TEXT], si);
        bu16(&relocs[WS_TEXT], ct);
    }
    for (size_t i = 0; i < m->num_data_relocs; i++) {
        const EmitReloc *r = &m->data_relocs[i];
        uint32_t si = (r->sym < m->num_syms) ? emit_to_coff[r->sym] : (uint32_t)-1;
        if (si == (uint32_t)-1) continue;
        int wi = WS_DATA;
        if (r->shndx == SECT_RODATA) wi = WS_RDATA;
        else if (r->shndx == SECT_TDATA) wi = WS_TDATA;
        else if (r->shndx == SECT_INIT_ARRAY) wi = WS_INIT;
        else if (r->shndx == SECT_FINI_ARRAY) wi = WS_FINI;
        uint16_t ct = elf_to_coff_reloc(r->type);
        bu32(&relocs[wi], (uint32_t)r->offset);
        bu32(&relocs[wi], si);
        bu16(&relocs[wi], ct);
    }
    if (want_unwind && pdata_rel.len)
        buffer_append(&relocs[WS_PDATA], pdata_rel.data, pdata_rel.len);

    /* Write file. */
    Buffer out;
    buffer_init(&out);
    bu16(&out, IMAGE_FILE_MACHINE_AMD64);
    bu16(&out, nsec);
    bu32(&out, 0); /* TimeDateStamp */
    bu32(&out, (uint32_t)sym_off);
    bu32(&out, (uint32_t)(symtab.len / COFF_SYM_SIZE));
    bu16(&out, 0); /* SizeOfOptionalHeader */
    bu16(&out, 0); /* Characteristics */

    for (int i = 0; i < WS_COUNT; i++) {
        if (!sec[i].present) continue;
        {
            char nm[8];
            memset(nm, 0, 8);
            size_t n = strlen(sec[i].name);
            if (n <= 8) {
                memcpy(nm, sec[i].name, n);
            } else {
                uint32_t off = 4;
                while (off < st.buf.len) {
                    const char *s = st.buf.data + off;
                    if (strcmp(s, sec[i].name) == 0) break;
                    off += (uint32_t)strlen(s) + 1;
                }
                nm[0] = '/';
                char tmp[16];
                snprintf(tmp, sizeof tmp, "%u", off);
                memcpy(nm + 1, tmp, strlen(tmp));
            }
            buffer_append(&out, nm, 8);
        }
        bu32(&out, (uint32_t)sec[i].size); /* VirtualSize */
        bu32(&out, 0); /* VirtualAddress */
        bu32(&out, sec[i].nobits ? 0 : (uint32_t)sec[i].size);
        bu32(&out, sec[i].nobits ? 0 : sec[i].file_off);
        bu32(&out, sec[i].nreloc ? sec[i].reloc_off : 0);
        bu32(&out, 0); /* PointerToLinenumbers */
        bu16(&out, (uint16_t)sec[i].nreloc);
        bu16(&out, 0);
        bu32(&out, sec[i].chars);
    }

    /* Pad to first section data. */
    while (out.len < hdr_sz) bu8(&out, 0);
    for (int i = 0; i < WS_COUNT; i++) {
        if (!sec[i].present || sec[i].nobits) continue;
        while (out.len < sec[i].file_off) bu8(&out, 0);
        if (sec[i].size && sec[i].data)
            buffer_append(&out, sec[i].data, sec[i].size);
        else if (sec[i].size)
            for (size_t z = 0; z < sec[i].size; z++) bu8(&out, 0);
    }
    for (int i = 0; i < WS_COUNT; i++) {
        if (!sec[i].present || !sec[i].nreloc) continue;
        while (out.len < sec[i].reloc_off) bu8(&out, 0);
        buffer_append(&out, relocs[i].data, relocs[i].len);
    }
    while (out.len < sym_off) bu8(&out, 0);
    buffer_append(&out, symtab.data, symtab.len);
    buffer_append(&out, st.buf.data, st.buf.len);
    (void)str_off;

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot write '%s'\n", path);
        free(emit_to_coff);
        return -1;
    }
    fwrite(out.data, 1, out.len, f);
    fclose(f);

    free(emit_to_coff);
    for (int i = 0; i < WS_COUNT; i++) buffer_free(&relocs[i]);
    buffer_free(&out);
    buffer_free(&symtab);
    buffer_free(&st.buf);
    buffer_free(&text_c); buffer_free(&rdata_c); buffer_free(&data_c);
    buffer_free(&tdata_c); buffer_free(&init_c); buffer_free(&fini_c);
    buffer_free(&fakecc_dbg);
    buffer_free(&pdata_b); buffer_free(&xdata_b); buffer_free(&pdata_rel);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Reader                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[64];
    uint32_t vsize;
    uint32_t rawsize;
    uint32_t rawptr;
    uint32_t relocptr;
    uint16_t nreloc;
    uint32_t chars;
    uint16_t mapped; /* SECT_* or 0 */
    size_t base_off; /* offset within mapped EmitModule section */
    int is_comdat;
    int comdat_index; /* index into EmitModule.comdats, or -1 */
    uint8_t comdat_sel;
    char *comdat_sym;
    char *assoc_name;
} RSect;

static int read_sect_name(const unsigned char *hdr, const unsigned char *strtab,
                          size_t strsz, char *out, size_t outsz) {
    if (hdr[0] == '/') {
        unsigned off = (unsigned)atoi((const char *)hdr + 1);
        if (!strtab || off >= strsz) return -1;
        snprintf(out, outsz, "%s", (const char *)strtab + off);
        return 0;
    }
    char tmp[9];
    memcpy(tmp, hdr, 8);
    tmp[8] = 0;
    snprintf(out, outsz, "%s", tmp);
    return 0;
}

static const char *sym_name_at(const unsigned char *sym,
                               const unsigned char *strtab, size_t strsz) {
    if (rd_u32(sym) != 0) {
        static char shortn[9];
        memcpy(shortn, sym, 8);
        shortn[8] = 0;
        /* trim trailing NULs already */
        return shortn;
    }
    uint32_t off = rd_u32(sym + 4);
    if (!strtab || off >= strsz) return "";
    return (const char *)strtab + off;
}

static int name_is(const char *n, const char *base) {
    if (strcmp(n, base) == 0) return 1;
    size_t bl = strlen(base);
    return strncmp(n, base, bl) == 0 && (n[bl] == '$' || n[bl] == '.');
}

static uint16_t map_sect_name(const char *n) {
    if (name_is(n, ".text") || name_is(n, "text")) return SECT_TEXT;
    if (name_is(n, ".rdata") || name_is(n, ".rodata") || name_is(n, ".rdata$"))
        return SECT_RODATA;
    if (name_is(n, ".data")) return SECT_DATA;
    if (name_is(n, ".bss")) return SECT_BSS;
    if (name_is(n, ".tbss")) return SECT_TBSS;
    if (name_is(n, ".tls") || name_is(n, ".tdata") || name_is(n, ".tls$"))
        return SECT_TDATA;
    if (name_is(n, ".CRT$XCU") || name_is(n, ".ctors") || name_is(n, ".init_array")
        || name_is(n, ".CRT$XCL") || name_is(n, ".CRT$XCT"))
        return SECT_INIT_ARRAY;
    if (name_is(n, ".CRT$XTX") || name_is(n, ".CRT$XTZ") || name_is(n, ".dtors")
        || name_is(n, ".fini_array"))
        return SECT_FINI_ARRAY;
    if (name_is(n, ".CRT$XL") || strncmp(n, ".CRT$XL", 7) == 0)
        return SECT_TLS_CALLBACKS;
    if (strcmp(n, ".pdata") == 0 || strncmp(n, ".pdata$", 7) == 0)
        return SECT_PDATA;
    if (strcmp(n, ".xdata") == 0 || strncmp(n, ".xdata$", 7) == 0)
        return SECT_XDATA;
    return 0;
}

int coff_read_object(const char *path, EmitModule *m) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "fakecc: cannot open '%s'\n", path); return -1; }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize < COFF_FILE_HDR) {
        fprintf(stderr, "fakecc: '%s' is too small for a COFF object\n", path);
        fclose(f); return -1;
    }
    unsigned char *buf = malloc((size_t)fsize);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, (size_t)fsize, f) != (size_t)fsize) {
        fprintf(stderr, "fakecc: short read on '%s'\n", path);
        free(buf); fclose(f); return -1;
    }
    fclose(f);
    int rc = coff_read_object_mem(buf, (size_t)fsize, path, m);
    free(buf);
    return rc;
}

int coff_read_object_mem(const void *data, size_t fsize, const char *path,
                         EmitModule *m) {
    const unsigned char *buf = (const unsigned char *)data;
    if (!buf || !m) return -1;
    if (!path) path = "(memory)";
    if (fsize < COFF_FILE_HDR) {
        fprintf(stderr, "fakecc: '%s' is too small for a COFF object\n", path);
        return -1;
    }

    uint16_t machine = rd_u16(buf);
    if (machine != IMAGE_FILE_MACHINE_AMD64) {
        if ((machine == 0 && rd_u16(buf + 2) == 0xFFFF)
            || (fsize >= 4 && memcmp(buf, "BC\xc0\xde", 4) == 0)
            || (fsize >= 4 && memcmp(buf, "\xde\xc0\x17\x0b", 4) == 0)) {
            fprintf(stderr,
                    "fakecc: '%s' is an unsupported LTCG/LTO object (compile with standard machine code, e.g. MSVC /GL- or clang -fno-lto)\n",
                    path);
            return -1;
        }
        fprintf(stderr, "fakecc: '%s' is not an AMD64 COFF object\n", path);
        return -1;
    }
    uint16_t nsec = rd_u16(buf + 2);
    uint32_t symptr = rd_u32(buf + 8);
    uint32_t nsyms = rd_u32(buf + 12);
    uint16_t opthdr = rd_u16(buf + 16);
    if (opthdr != 0) {
        fprintf(stderr, "fakecc: '%s' looks like a PE image, not a COFF object\n",
                path);
        return -1;
    }
    size_t shoff = COFF_FILE_HDR;
    if (shoff + (size_t)nsec * COFF_SECT_HDR > (size_t)fsize
        || (size_t)symptr + (size_t)nsyms * COFF_SYM_SIZE > (size_t)fsize) {
        fprintf(stderr, "fakecc: '%s' has truncated COFF headers\n", path);
        return -1;
    }

    const unsigned char *strtab = NULL;
    size_t strsz = 0;
    if (nsyms) {
        size_t stroff = (size_t)symptr + (size_t)nsyms * COFF_SYM_SIZE;
        if (stroff + 4 <= (size_t)fsize) {
            strsz = rd_u32(buf + stroff);
            if (stroff + strsz <= (size_t)fsize) {
                strtab = buf + stroff;
            }
        }
    }

    emit_module_init(m);

    RSect *rs = calloc(nsec ? nsec : 1, sizeof(RSect));
    if (!rs) return -1;

    size_t text_len = 0, rdata_len = 0, data_len = 0, bss_len = 0;
    size_t tdata_len = 0, tbss_len = 0, init_len = 0, fini_len = 0, cb_len = 0;
    int next_cd = 0;

    for (uint16_t i = 0; i < nsec; i++) {
        const unsigned char *sh = buf + shoff + (size_t)i * COFF_SECT_HDR;
        read_sect_name(sh, strtab, strsz, rs[i].name, sizeof rs[i].name);
        rs[i].vsize = rd_u32(sh + 8);
        rs[i].rawsize = rd_u32(sh + 16);
        rs[i].rawptr = rd_u32(sh + 20);
        rs[i].relocptr = rd_u32(sh + 24);
        rs[i].nreloc = rd_u16(sh + 32);
        rs[i].chars = rd_u32(sh + 36);
        rs[i].is_comdat = (rs[i].chars & IMAGE_SCN_LNK_COMDAT) != 0;
        rs[i].comdat_index = -1;
        if (rs[i].is_comdat) rs[i].comdat_index = next_cd++;
        size_t sz = rs[i].vsize ? rs[i].vsize : rs[i].rawsize;

        if (strcmp(rs[i].name, ".drectve") == 0) {
            if (rs[i].rawsize && rs[i].rawptr + rs[i].rawsize <= (size_t)fsize)
                buffer_append(&m->drectve, (const char *)buf + rs[i].rawptr,
                              rs[i].rawsize);
            continue;
        }
        if (strcmp(rs[i].name, ".pdata") == 0
            || strncmp(rs[i].name, ".pdata$", 7) == 0) {
            rs[i].base_off = m->pdata.len;
            if (rs[i].rawsize && rs[i].rawptr + rs[i].rawsize <= (size_t)fsize)
                buffer_append(&m->pdata, (const char *)buf + rs[i].rawptr,
                              rs[i].rawsize);
            if (rs[i].rawsize)
                emit_module_add_span(m, SECT_PDATA, rs[i].base_off, rs[i].rawsize,
                                     rs[i].comdat_index);
            continue;
        }
        if (strcmp(rs[i].name, ".xdata") == 0
            || strncmp(rs[i].name, ".xdata$", 7) == 0) {
            rs[i].base_off = m->xdata.len;
            if (rs[i].rawsize && rs[i].rawptr + rs[i].rawsize <= (size_t)fsize)
                buffer_append(&m->xdata, (const char *)buf + rs[i].rawptr,
                              rs[i].rawsize);
            if (rs[i].rawsize)
                emit_module_add_span(m, SECT_XDATA, rs[i].base_off, rs[i].rawsize,
                                     rs[i].comdat_index);
            continue;
        }
        if (strcmp(rs[i].name, ".fakecc_dbg") == 0) {
            if (rs[i].rawsize && rs[i].rawptr + rs[i].rawsize <= (size_t)fsize) {
                if (debug_deserialize(m, buf + rs[i].rawptr, rs[i].rawsize) != 0)
                    fprintf(stderr,
                            "fakecc: warning: ignoring corrupt .fakecc_dbg in '%s'\n",
                            path);
            }
            continue;
        }

        uint16_t mapped = map_sect_name(rs[i].name);
        if (mapped == SECT_TDATA && (rs[i].chars & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
            mapped = SECT_TBSS;
        rs[i].mapped = mapped;
        if (!mapped) continue;

        switch (mapped) {
        case SECT_TEXT: rs[i].base_off = text_len; text_len += sz; break;
        case SECT_RODATA: rs[i].base_off = rdata_len; rdata_len += sz; break;
        case SECT_DATA: rs[i].base_off = data_len; data_len += sz; break;
        case SECT_BSS: rs[i].base_off = bss_len; bss_len += sz; break;
        case SECT_TDATA: rs[i].base_off = tdata_len; tdata_len += sz; break;
        case SECT_TBSS: rs[i].base_off = tbss_len; tbss_len += sz; break;
        case SECT_INIT_ARRAY: rs[i].base_off = init_len; init_len += sz; break;
        case SECT_FINI_ARRAY: rs[i].base_off = fini_len; fini_len += sz; break;
        case SECT_TLS_CALLBACKS: rs[i].base_off = cb_len; cb_len += sz; break;
        }
        /* Alignment from characteristics bits 20-23. */
        unsigned al = ((rs[i].chars >> 20) & 0xF);
        if (al >= 1) {
            size_t a = (size_t)1 << (al - 1);
            if (mapped == SECT_TEXT && a > m->text_align) m->text_align = a;
            if (mapped == SECT_RODATA && a > m->rodata_align) m->rodata_align = a;
            if (mapped == SECT_DATA && a > m->data_align) m->data_align = a;
            if (mapped == SECT_BSS && a > m->bss_align) m->bss_align = a;
            if (mapped == SECT_TDATA && a > m->tdata_align) m->tdata_align = a;
            if (mapped == SECT_TBSS && a > m->tbss_align) m->tbss_align = a;
            if (mapped == SECT_TLS_CALLBACKS && a > m->tls_callbacks_align) m->tls_callbacks_align = a;
        }
        if (sz)
            emit_module_add_span(m, mapped, rs[i].base_off, sz, rs[i].comdat_index);
    }

    if (text_len) {
        m->text.data = calloc(1, text_len); m->text.len = m->text.cap = text_len;
    }
    if (rdata_len) {
        m->rodata.data = calloc(1, rdata_len); m->rodata.len = m->rodata.cap = rdata_len;
    }
    if (data_len) {
        m->data.data = calloc(1, data_len); m->data.len = m->data.cap = data_len;
    }
    m->bss_size = bss_len;
    if (tdata_len) {
        m->tdata.data = calloc(1, tdata_len); m->tdata.len = m->tdata.cap = tdata_len;
    }
    m->tbss_size = tbss_len;
    if (init_len) {
        m->init_array.data = calloc(1, init_len);
        m->init_array.len = m->init_array.cap = init_len;
        m->init_prio = calloc(init_len / 8 + 1, sizeof(int));
        for (size_t i = 0; i < init_len / 8; i++)
            m->init_prio[i] = INIT_PRIO_DEFAULT;
    }
    if (fini_len) {
        m->fini_array.data = calloc(1, fini_len);
        m->fini_array.len = m->fini_array.cap = fini_len;
        m->fini_prio = calloc(fini_len / 8 + 1, sizeof(int));
        for (size_t i = 0; i < fini_len / 8; i++)
            m->fini_prio[i] = INIT_PRIO_DEFAULT;
    }
    if (cb_len) {
        m->tls_callbacks.data = calloc(1, cb_len);
        m->tls_callbacks.len = m->tls_callbacks.cap = cb_len;
    }

    for (uint16_t i = 0; i < nsec; i++) {
        if (!rs[i].mapped || !rs[i].rawsize) continue;
        if (rs[i].rawptr + rs[i].rawsize > (size_t)fsize) continue;
        char *dst = NULL;
        switch (rs[i].mapped) {
        case SECT_TEXT: dst = m->text.data; break;
        case SECT_RODATA: dst = m->rodata.data; break;
        case SECT_DATA: dst = m->data.data; break;
        case SECT_TDATA: dst = m->tdata.data; break;
        case SECT_INIT_ARRAY: dst = m->init_array.data; break;
        case SECT_FINI_ARRAY: dst = m->fini_array.data; break;
        case SECT_TLS_CALLBACKS: dst = m->tls_callbacks.data; break;
        default: break;
        }
        if (dst)
            memcpy(dst + rs[i].base_off, buf + rs[i].rawptr, rs[i].rawsize);
    }

    /* Symbol table → EmitModule.  coff_to_emit[i] = emit index or -1 for aux. */
    int *coff_to_emit = malloc((size_t)nsyms * sizeof(int));
    if (!coff_to_emit) { free(rs); return -1; }
    for (uint32_t i = 0; i < nsyms; i++) coff_to_emit[i] = -1;

    const unsigned char *symbase = buf + symptr;
    for (uint32_t i = 0; i < nsyms; ) {
        const unsigned char *sym = symbase + (size_t)i * COFF_SYM_SIZE;
        const char *name = sym_name_at(sym, strtab, strsz);
        /* Copy short name out of static buffer. */
        char namebuf[256];
        snprintf(namebuf, sizeof namebuf, "%s", name);
        name = namebuf;

        uint32_t value = rd_u32(sym + 8);
        int16_t secno = (int16_t)rd_u16(sym + 12);
        uint16_t typ = rd_u16(sym + 14);
        uint8_t sclass = sym[16];
        uint8_t naux = sym[17];

        if (sclass == IMAGE_SYM_CLASS_FILE) {
            i += 1 + naux;
            continue;
        }

        /* Section symbol */
        if (sclass == IMAGE_SYM_CLASS_STATIC && secno > 0
            && (typ == 0) && name[0] == '.') {
            uint16_t sh = SECT_TEXT;
            size_t val = 0;
            if ((uint16_t)secno <= nsec) {
                uint16_t ms = map_sect_name(rs[secno - 1].name);
                if (ms) sh = ms;
                val = rs[secno - 1].base_off;
            }
            int ei = emit_module_add_symbol(m, namebuf, STB_LOCAL, STT_SECTION,
                                            sh, val, 0);
            coff_to_emit[i] = ei;
            /* COMDAT aux is the first aux of the section symbol when
             * IMAGE_SCN_LNK_COMDAT is set on that section. */
            if (naux && secno >= 1 && (uint16_t)secno <= nsec) {
                RSect *r = &rs[secno - 1];
                if (r->is_comdat) {
                    const unsigned char *aux = sym + COFF_SYM_SIZE;
                    /* Aux format for COMDAT: Length, NumberOfRelocations,
                     * NumberOfLinenumbers, CheckSum, Number, Selection */
                    uint8_t sel = aux[14];
                    uint16_t assoc = rd_u16(aux + 12);
                    r->comdat_sel = sel;
                    if (sel == IMAGE_COMDAT_SELECT_ASSOCIATIVE
                        && assoc >= 1 && assoc <= nsec) {
                        r->assoc_name = xstrdup(rs[assoc - 1].name);
                    }
                }
            }
            i += 1 + naux;
            continue;
        }

        uint8_t binding = STB_GLOBAL;
        uint8_t st_type = STT_NOTYPE;
        uint16_t shndx = SECT_UNDEF;
        size_t sval = value;
        size_t ssize = 0;

        if (typ == 0x20) st_type = STT_FUNC;
        else if (sclass == IMAGE_SYM_CLASS_EXTERNAL) st_type = STT_OBJECT;

        if (sclass == IMAGE_SYM_CLASS_WEAK_EXTERNAL) {
            binding = STB_WEAK;
            shndx = SECT_UNDEF;
        } else if (secno == 0 && sclass == IMAGE_SYM_CLASS_EXTERNAL && value != 0) {
            /* Common symbol: Value = size */
            shndx = SHN_COMMON;
            ssize = value;
            sval = 8; /* alignment guess */
            binding = STB_GLOBAL;
            st_type = STT_OBJECT;
        } else if (secno == 0) {
            shndx = SECT_UNDEF;
            binding = (sclass == IMAGE_SYM_CLASS_STATIC) ? STB_LOCAL : STB_GLOBAL;
        } else if (secno > 0 && (uint16_t)secno <= nsec) {
            RSect *r = &rs[secno - 1];
            shndx = r->mapped ? r->mapped : SECT_TEXT;
            sval = (size_t)value + r->base_off;
            binding = (sclass == IMAGE_SYM_CLASS_STATIC) ? STB_LOCAL : STB_GLOBAL;
            if (r->is_comdat && !r->comdat_sym && name[0] && name[0] != '.')
                r->comdat_sym = xstrdup(name);
        } else {
            i += 1 + naux;
            continue;
        }

        int ei = emit_module_add_symbol(m, name[0] ? name : NULL, binding,
                                        st_type, shndx, sval, ssize);
        coff_to_emit[i] = ei;
        i += 1 + naux;
    }

    /* Record COMDAT metadata. */
    for (uint16_t i = 0; i < nsec; i++) {
        if (!rs[i].is_comdat) continue;
        emit_module_add_comdat(m, rs[i].name,
                               rs[i].comdat_sel ? rs[i].comdat_sel
                                                : IMAGE_COMDAT_SELECT_ANY,
                               rs[i].comdat_sym, rs[i].assoc_name);
    }

    /* Relocations. */
    for (uint16_t si = 0; si < nsec; si++) {
        if (!rs[si].nreloc || !rs[si].relocptr) continue;
        int is_pdata = strcmp(rs[si].name, ".pdata") == 0
            || strncmp(rs[si].name, ".pdata$", 7) == 0;
        int is_xdata = strcmp(rs[si].name, ".xdata") == 0
            || strncmp(rs[si].name, ".xdata$", 7) == 0;
        if (!rs[si].mapped && !is_pdata && !is_xdata) continue;

        size_t rsz = (size_t)rs[si].nreloc * COFF_RELOC_SZ;
        if ((size_t)rs[si].relocptr + rsz > (size_t)fsize) continue;
        char *secdata = NULL;
        size_t seclen = 0;
        if (is_pdata) {
            secdata = m->pdata.data; seclen = m->pdata.len;
        } else if (is_xdata) {
            secdata = m->xdata.data; seclen = m->xdata.len;
        } else switch (rs[si].mapped) {
        case SECT_TEXT: secdata = m->text.data; seclen = m->text.len; break;
        case SECT_RODATA: secdata = m->rodata.data; seclen = m->rodata.len; break;
        case SECT_DATA: secdata = m->data.data; seclen = m->data.len; break;
        case SECT_TDATA: secdata = m->tdata.data; seclen = m->tdata.len; break;
        case SECT_INIT_ARRAY: secdata = m->init_array.data; seclen = m->init_array.len; break;
        case SECT_FINI_ARRAY: secdata = m->fini_array.data; seclen = m->fini_array.len; break;
        case SECT_TLS_CALLBACKS: secdata = m->tls_callbacks.data; seclen = m->tls_callbacks.len; break;
        default: break;
        }

        for (uint16_t ri = 0; ri < rs[si].nreloc; ri++) {
            const unsigned char *r = buf + rs[si].relocptr
                                     + (size_t)ri * COFF_RELOC_SZ;
            uint32_t off = rd_u32(r);
            uint32_t symi = rd_u32(r + 4);
            uint16_t rty = rd_u16(r + 8);
            if (symi >= nsyms || coff_to_emit[symi] < 0) continue;
            int width = reloc_width(rty);
            int32_t addend = 0;
            size_t abs_off = rs[si].base_off + off;
            if (secdata && abs_off + (size_t)width <= seclen) {
                if (width == 8) {
                    int64_t v;
                    memcpy(&v, secdata + abs_off, 8);
                    addend = (int32_t)v;
                    memset(secdata + abs_off, 0, 8);
                } else if (width == 4) {
                    addend = rd_i32((unsigned char *)secdata + abs_off);
                    memset(secdata + abs_off, 0, 4);
                } else if (width == 2) {
                    addend = (int16_t)rd_u16((unsigned char *)secdata + abs_off);
                    memset(secdata + abs_off, 0, 2);
                }
            }
            addend += reloc_extra_addend(rty);
            uint32_t ety = coff_to_elf_reloc(rty);
            if (is_pdata) {
                emit_module_add_pdata_reloc(m, abs_off, ety, coff_to_emit[symi],
                                            addend);
            } else if (is_xdata) {
                emit_module_add_xdata_reloc(m, abs_off, ety, coff_to_emit[symi],
                                            addend);
            } else if (rs[si].mapped == SECT_TEXT) {
                emit_module_add_reloc(m, abs_off, ety, coff_to_emit[symi], addend);
            } else {
                emit_module_add_data_reloc(m, abs_off, ety, coff_to_emit[symi],
                                           addend);
                m->data_relocs[m->num_data_relocs - 1].shndx = rs[si].mapped;
            }
        }
    }

    for (uint16_t i = 0; i < nsec; i++) {
        free(rs[i].comdat_sym);
        free(rs[i].assoc_name);
    }
    free(rs);
    free(coff_to_emit);
    return 0;
}
