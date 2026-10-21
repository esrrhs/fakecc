/* COFF archives (.a / .lib), Windows import libraries, and lazy member
 * extraction for the PE linker.
 *
 * Two import-library shapes are understood:
 *   - GNU dlltool objects (what `gcc -Wl,--out-implib` writes): a head
 *     object, one object per symbol, and a tail that holds the DLL name
 *     and the ILT/IAT nulls.  ld sorts those members by filename.
 *   - MSVC short import objects (IMPORT_OBJECT_HEADER, sig2 = 0xFFFF).
 *
 * A static archive member is a normal AMD64 COFF object and is read only
 * when one of its symbols is still undefined.
 */

#include "fakecc/pe.h"
#include "fakecc/emit.h"
#include "fakecc/common.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AR_MAGIC "!<arch>\n"

static uint16_t ru16(const unsigned char *p) {
    uint16_t v; memcpy(&v, p, 2); return v;
}
static uint32_t ru32(const unsigned char *p) {
    uint32_t v; memcpy(&v, p, 4); return v;
}
static uint32_t ru32be(const unsigned char *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static uint64_t ru64be(const unsigned char *p) {
    return ((uint64_t)ru32be(p) << 32) | ru32be(p + 4);
}

static void wu16(Buffer *b, uint16_t v) { buffer_append(b, (const char *)&v, 2); }
static void wu32(Buffer *b, uint32_t v) { buffer_append(b, (const char *)&v, 4); }

/* ------------------------------------------------------------------ */
/* Archive                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t hdr;       /* file offset of the 60-byte header */
    uint32_t size;      /* payload bytes */
    char *name;
} ArMem;

typedef struct {
    char *name;
    int mem;
} ArSym;

typedef struct {
    char *head;         /* _head_* symbol */
    char *dll;
} DllCache;

typedef struct {
    unsigned char *data;
    size_t len;
    char *path;
    ArMem *mem;
    size_t nmem;
    ArSym *syms;
    size_t nsyms;
    DllCache *dlls;
    size_t ndlls;
    int is_dll;
    PeImport *dll_imps;
    size_t n_dll_imps;
    uint8_t *taken;
} Archive;

static void archive_free(Archive *a) {
    if (!a) return;
    free(a->data);
    free(a->path);
    for (size_t i = 0; i < a->nmem; i++) free(a->mem[i].name);
    free(a->mem);
    for (size_t i = 0; i < a->nsyms; i++) free(a->syms[i].name);
    free(a->syms);
    for (size_t i = 0; i < a->ndlls; i++) {
        free(a->dlls[i].head);
        free(a->dlls[i].dll);
    }
    free(a->dlls);
    pe_imports_free(a->dll_imps, a->n_dll_imps);
    free(a->taken);
    memset(a, 0, sizeof *a);
}

static int hdr_magic_ok(const unsigned char *h) {
    return h[58] == '`' && h[59] == '\n';
}

static uint32_t hdr_size(const unsigned char *h) {
    char tmp[16];
    memcpy(tmp, h + 48, 10);
    tmp[10] = 0;
    return (uint32_t)strtoul(tmp, NULL, 10);
}

static char *name_from_raw(const char raw[16], const char *ln, size_t lnlen) {
    char buf[17];
    memcpy(buf, raw, 16);
    buf[16] = 0;
    size_t n = strlen(buf);
    while (n && buf[n - 1] == ' ') buf[--n] = 0;
    if (n >= 2 && buf[0] == '/' && buf[1] != '/' && buf[1] != ' ' && isdigit((unsigned char)buf[1])) {
        unsigned off = (unsigned)atoi(buf + 1);
        if (!ln || off >= lnlen) return xstrdup(buf);
        const char *s = ln + off;
        const char *e = s;
        while ((size_t)(e - ln) < lnlen && *e && *e != '/' && *e != '\n') e++;
        size_t L = (size_t)(e - s);
        char *o = malloc(L + 1);
        if (!o) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        memcpy(o, s, L);
        o[L] = 0;
        return o;
    }
    if (n && buf[n - 1] == '/') buf[--n] = 0;
    return xstrdup(buf);
}

static int mem_index_at(const Archive *a, uint64_t off) {
    for (size_t i = 0; i < a->nmem; i++)
        if ((uint64_t)a->mem[i].hdr == off) return (int)i;
    return -1;
}

static void arch_add_sym(Archive *a, const char *name, int mem) {
    if (!name || !*name || mem < 0) return;
    if (a->nsyms == 0) {
        /* first symbol */
    }
    a->syms = xrealloc(a->syms, (a->nsyms + 1) * sizeof(ArSym));
    a->syms[a->nsyms].name = xstrdup(name);
    a->syms[a->nsyms].mem = mem;
    a->nsyms++;
}

/* GNU: little-endian count + offsets.  MSVC first linker member: big-endian.
 * wide64 selects the /SYM64/ table.  Returns the number of offsets that
 * land on a real member header. */
static int try_symtab(Archive *a, const unsigned char *p, uint32_t size,
                      int be, int wide, int commit) {
    if (size < 4) return 0;
    uint64_t n;
    size_t ent = wide ? 8 : 4;
    if (wide) {
        if (size < 8) return 0;
        n = be ? ru64be(p) : ((uint64_t)ru32(p) | ((uint64_t)ru32(p + 4) << 32));
    } else {
        n = be ? ru32be(p) : ru32(p);
    }
    if (!n || n > 1000000) return 0;
    size_t table = (wide ? 8 : 4) + (size_t)n * ent;
    if (table > size) return 0;
    int good = 0;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t off;
        const unsigned char *ep = p + (wide ? 8 : 4) + (size_t)i * ent;
        if (wide) off = be ? ru64be(ep) : (uint64_t)ru32(ep) | ((uint64_t)ru32(ep + 4) << 32);
        else off = be ? ru32be(ep) : ru32(ep);
        if (off + 60 <= a->len && hdr_magic_ok(a->data + off) && mem_index_at(a, off) >= 0)
            good++;
    }
    if (!commit || good != (int)n) return good;
    const char *s = (const char *)p + table;
    const char *end = (const char *)p + size;
    for (uint64_t i = 0; i < n; i++) {
        if (s >= end) break;
        uint64_t off;
        const unsigned char *ep = p + (wide ? 8 : 4) + (size_t)i * ent;
        if (wide) off = be ? ru64be(ep) : (uint64_t)ru32(ep) | ((uint64_t)ru32(ep + 4) << 32);
        else off = be ? ru32be(ep) : ru32(ep);
        arch_add_sym(a, s, mem_index_at(a, off));
        s += strlen(s) + 1;
    }
    return good;
}

/* MSVC second linker member: LE member count, offsets, symbol count, 1-based
 * indices, then strings. */
static int try_second_linker(Archive *a, const unsigned char *p, uint32_t size) {
    if (size < 8) return 0;
    uint32_t nm = ru32(p);
    if (!nm || nm > 1000000) return 0;
    size_t off_bytes = 4u + (size_t)nm * 4u;
    if (off_bytes + 4 > size) return 0;
    uint32_t ns = ru32(p + off_bytes);
    if (!ns || ns > 1000000) return 0;
    size_t idx_at = off_bytes + 4;
    if (idx_at + (size_t)ns * 2 > size) return 0;
    int good = 0;
    for (uint32_t i = 0; i < ns; i++) {
        uint16_t ix = ru16(p + idx_at + (size_t)i * 2);
        if (!ix || ix > nm) continue;
        uint32_t off = ru32(p + 4 + (size_t)(ix - 1) * 4);
        if (off + 60 <= a->len && hdr_magic_ok(a->data + off)) good++;
    }
    if (good != (int)ns) return 0;
    const char *s = (const char *)p + idx_at + (size_t)ns * 2;
    const char *end = (const char *)p + size;
    for (uint32_t i = 0; i < ns && s < end; i++) {
        uint16_t ix = ru16(p + idx_at + (size_t)i * 2);
        uint32_t off = (ix && ix <= nm) ? ru32(p + 4 + (size_t)(ix - 1) * 4) : 0;
        arch_add_sym(a, s, mem_index_at(a, off));
        s += strlen(s) + 1;
    }
    return 1;
}

static int rva_to_off(const unsigned char *buf, size_t n, uint32_t pe,
                      uint32_t rva, uint32_t *out) {
    uint16_t nsec = ru16(buf + pe + 6);
    uint16_t opt = ru16(buf + pe + 20);
    size_t sh = (size_t)pe + 24 + opt;
    for (uint16_t i = 0; i < nsec; i++) {
        if (sh + 40 > n) return -1;
        const unsigned char *s = buf + sh + (size_t)i * 40;
        uint32_t va = ru32(s + 12);
        uint32_t vsz = ru32(s + 8);
        uint32_t raw = ru32(s + 20);
        uint32_t rsz = ru32(s + 16);
        uint32_t span = vsz > rsz ? vsz : rsz;
        if (rva >= va && rva < va + span) {
            *out = raw + (rva - va);
            return 0;
        }
    }
    return -1;
}

static int section_is_exec(const unsigned char *buf, size_t n, uint32_t pe,
                           uint32_t rva) {
    uint16_t nsec = ru16(buf + pe + 6);
    uint16_t opt = ru16(buf + pe + 20);
    size_t sh = (size_t)pe + 24 + opt;
    for (uint16_t i = 0; i < nsec; i++) {
        if (sh + 40 > n) return 0;
        const unsigned char *s = buf + sh + (size_t)i * 40;
        uint32_t va = ru32(s + 12);
        uint32_t vsz = ru32(s + 8);
        uint32_t rsz = ru32(s + 16);
        uint32_t span = vsz > rsz ? vsz : rsz;
        if (rva >= va && rva < va + span)
            return (ru32(s + 36) & 0x20000000u) != 0;
    }
    return 0;
}

static int read_dll_exports(Archive *a) {
    const unsigned char *buf = a->data;
    size_t n = a->len;
    if (n < 0x40 || buf[0] != 'M' || buf[1] != 'Z') return -1;
    uint32_t pe = ru32(buf + 0x3c);
    if ((size_t)pe + 24 + 240 > n) return -1;
    if (memcmp(buf + pe, "PE\0\0", 4) != 0) return -1;
    if (ru16(buf + pe + 4) != IMAGE_FILE_MACHINE_AMD64) return -1;
    if (ru16(buf + pe + 24) != 0x20B) return -1;
    uint32_t exp_rva = ru32(buf + pe + 24 + 112);
    uint32_t exp_sz = ru32(buf + pe + 24 + 116);
    if (!exp_rva || !exp_sz) return 0;
    uint32_t exp_off = 0;
    if (rva_to_off(buf, n, pe, exp_rva, &exp_off) != 0 || exp_off + 40 > n)
        return -1;
    const unsigned char *ed = buf + exp_off;
    uint32_t name_rva = ru32(ed + 12);
    uint32_t nnames = ru32(ed + 24);
    uint32_t names_rva = ru32(ed + 32);
    uint32_t ords_rva = ru32(ed + 36);
    uint32_t funcs_rva = ru32(ed + 28);
    char dllbuf[256];
    const char *dll = NULL;
    uint32_t noff;
    if (name_rva && rva_to_off(buf, n, pe, name_rva, &noff) == 0 && noff < n) {
        snprintf(dllbuf, sizeof dllbuf, "%s", (const char *)buf + noff);
        dll = dllbuf;
    }
    if (!dll || !dll[0]) {
        const char *base = a->path;
        for (const char *p = a->path; *p; p++)
            if (*p == '/' || *p == '\\') base = p + 1;
        snprintf(dllbuf, sizeof dllbuf, "%s", base);
        dll = dllbuf;
    }
    uint32_t names_off, ords_off, funcs_off;
    if (rva_to_off(buf, n, pe, names_rva, &names_off) != 0) return -1;
    if (rva_to_off(buf, n, pe, ords_rva, &ords_off) != 0) return -1;
    if (rva_to_off(buf, n, pe, funcs_rva, &funcs_off) != 0) return -1;
    for (uint32_t i = 0; i < nnames; i++) {
        if (names_off + (i + 1) * 4 > n || ords_off + (i + 1) * 2 > n) break;
        uint32_t nrva = ru32(buf + names_off + i * 4);
        uint16_t ord = ru16(buf + ords_off + i * 2);
        uint32_t soff;
        if (rva_to_off(buf, n, pe, nrva, &soff) != 0 || soff >= n) continue;
        if (funcs_off + ((uint32_t)ord + 1) * 4 > n) continue;
        uint32_t frva = ru32(buf + funcs_off + (uint32_t)ord * 4);
        if (frva >= exp_rva && frva < exp_rva + exp_sz) continue; /* forwarder */
        PeImport im;
        memset(&im, 0, sizeof im);
        im.dll = xstrdup(dll);
        im.name = xstrdup((const char *)buf + soff);
        im.is_data = !section_is_exec(buf, n, pe, frva);
        im.hint = (int)i;
        a->dll_imps = xrealloc(a->dll_imps, (a->n_dll_imps + 1) * sizeof(PeImport));
        a->dll_imps[a->n_dll_imps++] = im;
    }
    a->is_dll = 1;
    return 0;
}

static int archive_load(Archive *a, const char *path) {
    memset(a, 0, sizeof *a);
    a->path = xstrdup(path);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot open '%s'\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 8) {
        fprintf(stderr, "fakecc: '%s' is not a COFF archive or PE image\n", path);
        fclose(f);
        return -1;
    }
    a->data = malloc((size_t)sz);
    a->len = (size_t)sz;
    if (!a->data || fread(a->data, 1, a->len, f) != a->len) {
        fprintf(stderr, "fakecc: short read on '%s'\n", path);
        fclose(f);
        return -1;
    }
    fclose(f);
    if (a->data[0] == 'M' && a->data[1] == 'Z')
        return read_dll_exports(a);
    if (memcmp(a->data, AR_MAGIC, 8) != 0) {
        fprintf(stderr, "fakecc: '%s' is not a COFF archive or PE image\n", path);
        return -1;
    }
    size_t off = 8;
    while (off + 60 <= a->len) {
        const unsigned char *h = a->data + off;
        if (!hdr_magic_ok(h)) break;
        uint32_t psz = hdr_size(h);
        if (off + 60 + psz > a->len) break;
        a->mem = xrealloc(a->mem, (a->nmem + 1) * sizeof(ArMem));
        ArMem *m = &a->mem[a->nmem];
        memset(m, 0, sizeof *m);
        m->hdr = (uint32_t)off;
        m->size = psz;
        char raw[16];
        memcpy(raw, h, 16);
        m->name = name_from_raw(raw, NULL, 0);
        a->nmem++;
        off += 60 + psz;
        if (off & 1) off++;
    }
    const char *ln = NULL;
    size_t lnlen = 0;
    for (size_t i = 0; i < a->nmem; i++) {
        if (strcmp(a->mem[i].name, "//") == 0) {
            ln = (const char *)a->data + a->mem[i].hdr + 60;
            lnlen = a->mem[i].size;
            break;
        }
    }
    if (ln) {
        for (size_t i = 0; i < a->nmem; i++) {
            char raw[16];
            memcpy(raw, a->data + a->mem[i].hdr, 16);
            if (raw[0] == '/' && isdigit((unsigned char)raw[1])) {
                free(a->mem[i].name);
                a->mem[i].name = name_from_raw(raw, ln, lnlen);
            }
        }
    }
    for (size_t i = 0; i < a->nmem && a->nsyms == 0; i++) {
        const char *nm = a->mem[i].name;
        const unsigned char *p = a->data + a->mem[i].hdr + 60;
        uint32_t psz = a->mem[i].size;
        if (strcmp(nm, "/SYM64/") == 0) {
            try_symtab(a, p, psz, 0, 1, 1);
        } else if (strcmp(nm, "/") == 0) {
            int le = try_symtab(a, p, psz, 0, 0, 0);
            int be = try_symtab(a, p, psz, 1, 0, 0);
            if (le > 0 && le >= be) try_symtab(a, p, psz, 0, 0, 1);
            else if (be > 0) try_symtab(a, p, psz, 1, 0, 1);
            else try_second_linker(a, p, psz);
        }
    }
    a->taken = calloc(a->nmem ? a->nmem : 1, 1);
    return 0;
}

static int arch_find(const Archive *a, const char *name) {
    for (size_t i = 0; i < a->nsyms; i++)
        if (strcmp(a->syms[i].name, name) == 0) return a->syms[i].mem;
    if (a->is_dll) {
        for (size_t i = 0; i < a->n_dll_imps; i++)
            if (strcmp(a->dll_imps[i].name, name) == 0) return (int)i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Import-object parse                                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[256];
    int16_t sec;
    uint8_t sclass;
} CSym;

typedef struct {
    char name[32];
    uint32_t rawsz;
    const unsigned char *raw;
} CSec;

static int parse_coff_min(const unsigned char *buf, uint32_t n,
                          CSec *sec, int *nsec, CSym *sym, int *nsym) {
    *nsec = 0;
    *nsym = 0;
    if (n < 20) return -1;
    if (ru16(buf) != IMAGE_FILE_MACHINE_AMD64) return -1;
    if (ru16(buf + 16) != 0) return -1;
    int ns = ru16(buf + 2);
    uint32_t symptr = ru32(buf + 8);
    uint32_t nsyms = ru32(buf + 12);
    if (ns < 0 || ns > 16 || nsyms > 64) return -1;
    if (20u + (uint32_t)ns * 40u > n) return -1;
    const unsigned char *strtab = NULL;
    size_t strsz = 0;
    if (nsyms && (size_t)symptr + (size_t)nsyms * 18 + 4 <= n) {
        size_t stroff = (size_t)symptr + (size_t)nsyms * 18;
        strsz = ru32(buf + stroff);
        if (stroff + strsz <= n) strtab = buf + stroff;
    }
    for (int i = 0; i < ns; i++) {
        const unsigned char *sh = buf + 20 + (uint32_t)i * 40;
        char nm[16];
        if (sh[0] == '/' && strtab) {
            unsigned o = (unsigned)atoi((const char *)sh + 1);
            snprintf(sec[i].name, sizeof sec[i].name, "%s",
                     (o < strsz) ? (const char *)strtab + o : "");
        } else {
            memcpy(nm, sh, 8);
            nm[8] = 0;
            snprintf(sec[i].name, sizeof sec[i].name, "%s", nm);
        }
        sec[i].rawsz = ru32(sh + 16);
        uint32_t rp = ru32(sh + 20);
        sec[i].raw = (sec[i].rawsz && (size_t)rp + sec[i].rawsz <= n)
                     ? buf + rp : NULL;
    }
    *nsec = ns;
    if (!nsyms || (size_t)symptr + (size_t)nsyms * 18 > n) return 0;
    for (uint32_t i = 0; i < nsyms && *nsym < 64; ) {
        const unsigned char *s = buf + symptr + (size_t)i * 18;
        uint8_t naux = s[17];
        char nb[256];
        if (ru32(s) != 0) {
            memcpy(nb, s, 8);
            nb[8] = 0;
        } else {
            uint32_t o = ru32(s + 4);
            snprintf(nb, sizeof nb, "%s",
                     (strtab && o < strsz) ? (const char *)strtab + o : "");
        }
        snprintf(sym[*nsym].name, sizeof sym[*nsym].name, "%s", nb);
        sym[*nsym].sec = (int16_t)ru16(s + 12);
        sym[*nsym].sclass = s[16];
        (*nsym)++;
        i += 1u + naux;
    }
    return 0;
}

static int sec_is_idata(const char *n) {
    return strncmp(n, ".idata", 6) == 0;
}

static const CSec *find_sec(const CSec *sec, int nsec, const char *name) {
    for (int i = 0; i < nsec; i++)
        if (strcmp(sec[i].name, name) == 0) return &sec[i];
    return NULL;
}

static char *idata6_name(const CSec *s, int *hint) {
    *hint = 0;
    if (!s || !s->raw || s->rawsz < 3) return NULL;
    *hint = (int)ru16(s->raw);
    if (s->raw[2] == 0) return NULL;
    return xstrdup((const char *)s->raw + 2);
}

static char *idata7_string(const CSec *s) {
    if (!s || !s->raw || !s->rawsz) return NULL;
    if (s->raw[0] == 0) return NULL;
    /* A reloc-only slot is four zero bytes; a DLL name is printable. */
    if (s->raw[0] < 32 || s->raw[0] > 126) return NULL;
    return xstrdup((const char *)s->raw);
}

static const char *cached_dll(Archive *a, const char *head) {
    for (size_t i = 0; i < a->ndlls; i++)
        if (strcmp(a->dlls[i].head, head) == 0) return a->dlls[i].dll;
    return NULL;
}

static void cache_dll(Archive *a, const char *head, const char *dll) {
    a->dlls = xrealloc(a->dlls, (a->ndlls + 1) * sizeof(DllCache));
    a->dlls[a->ndlls].head = xstrdup(head);
    a->dlls[a->ndlls].dll = xstrdup(dll);
    a->ndlls++;
}

/* Follow _head_* → the tail symbol it references → that member's .idata$7. */
static char *dll_via_head(Archive *a, const char *head_sym) {
    const char *hit = cached_dll(a, head_sym);
    if (hit) return xstrdup(hit);
    int hm = arch_find(a, head_sym);
    if (hm < 0 || a->is_dll) return NULL;
    const unsigned char *p = a->data + a->mem[hm].hdr + 60;
    CSec sec[16];
    CSym sym[64];
    int nsec = 0, nsym = 0;
    if (parse_coff_min(p, a->mem[hm].size, sec, &nsec, sym, &nsym) != 0)
        return NULL;
    const char *iname = NULL;
    for (int i = 0; i < nsym; i++) {
        if (sym[i].sec == 0 && sym[i].sclass == IMAGE_SYM_CLASS_EXTERNAL
            && sym[i].name[0] && strncmp(sym[i].name, "_head_", 6) != 0) {
            iname = sym[i].name;
            break;
        }
    }
    if (!iname) return NULL;
    int tm = arch_find(a, iname);
    if (tm < 0) return NULL;
    const unsigned char *tp = a->data + a->mem[tm].hdr + 60;
    CSec tsec[16];
    CSym tsym[64];
    int tnsec = 0, tnsym = 0;
    if (parse_coff_min(tp, a->mem[tm].size, tsec, &tnsec, tsym, &tnsym) != 0)
        return NULL;
    const CSec *s7 = find_sec(tsec, tnsec, ".idata$7");
    char *dll = idata7_string(s7);
    if (dll) cache_dll(a, head_sym, dll);
    return dll;
}

static int ascii_lower(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int icmp(const char *a, const char *b) {
    while (*a && *b) {
        int ca = ascii_lower((unsigned char)*a++);
        int cb = ascii_lower((unsigned char)*b++);
        if (ca != cb) return ca - cb;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static int add_import(PeImport **imps, size_t *n, const char *dll, const char *name,
                      int is_data, int ordinal, int hint) {
    for (size_t i = 0; i < *n; i++) {
        if ((*imps)[i].is_data == is_data && icmp((*imps)[i].dll, dll) == 0
            && strcmp((*imps)[i].name, name) == 0)
            return 0;
    }
    *imps = xrealloc(*imps, (*n + 1) * sizeof(PeImport));
    PeImport *im = &(*imps)[*n];
    im->dll = xstrdup(dll);
    im->name = xstrdup(name);
    im->is_data = is_data;
    im->ordinal = ordinal;
    im->hint = hint;
    (*n)++;
    return 1;
}

/* Returns 1 if this member is import machinery (consumed, not linked as
 * an object), 0 if the caller should read it as a normal COFF object. */
static int take_import_member(Archive *a, int mi, const char *want,
                              int want_is_imp_alias,
                              PeImport **imps, size_t *nimps,
                              char ***defined, size_t *ndef) {
    const unsigned char *p = a->data + a->mem[mi].hdr + 60;
    uint32_t sz = a->mem[mi].size;
    if (sz >= 20 && ru16(p) == 0 && ru16(p + 2) == 0xFFFF) {
        uint16_t machine = ru16(p + 6);
        if (machine != IMAGE_FILE_MACHINE_AMD64 && machine != 0) return 0;
        uint16_t hint = ru16(p + 16);
        uint16_t flags = ru16(p + 18);
        int type = flags & 3;
        int nametype = (flags >> 2) & 7;
        const char *sym = (const char *)p + 20;
        size_t sl = 0;
        while (sl < sz - 20 && sym[sl]) sl++;
        if (20 + sl + 1 >= sz) return 1;
        const char *dll = sym + sl + 1;
        int ordinal = (nametype == 0) ? (int)hint : 0;
        int is_data = type != 0;
        add_import(imps, nimps, dll, sym, is_data, ordinal, ordinal ? 0 : (int)hint);
        (void)defined; (void)ndef; (void)want; (void)want_is_imp_alias;
        return 1;
    }
    CSec sec[16];
    CSym sym[64];
    int nsec = 0, nsym = 0;
    if (parse_coff_min(p, sz, sec, &nsec, sym, &nsym) != 0) return 0;
    int any_idata = 0;
    for (int i = 0; i < nsec; i++)
        if (sec_is_idata(sec[i].name)) any_idata = 1;
    if (!any_idata) return 0;

    const char *head = NULL;
    const char *imp = NULL;
    const char *code = NULL;
    for (int i = 0; i < nsym; i++) {
        if (sym[i].sclass != IMAGE_SYM_CLASS_EXTERNAL) continue;
        if (sym[i].sec == 0) {
            if (strncmp(sym[i].name, "_head_", 6) == 0) head = sym[i].name;
            continue;
        }
        if (strncmp(sym[i].name, "__imp_", 6) == 0) imp = sym[i].name;
        else if (strncmp(sym[i].name, "__nm_", 5) == 0) continue;
        else if (strncmp(sym[i].name, "_head_", 6) == 0) continue;
        else if (sym[i].name[0] && sym[i].name[0] != '.') {
            const CSec *sc = (sym[i].sec >= 1 && sym[i].sec <= nsec)
                             ? &sec[sym[i].sec - 1] : NULL;
            if (sc && strcmp(sc->name, ".text") == 0 && sc->rawsz > 0)
                code = sym[i].name;
        }
    }
    /* Head / tail metadata: nothing to import on its own. */
    if (!imp && !code) return 1;
    const char *exp = NULL;
    int is_data = 1;
    if (code) {
        exp = code;
        is_data = 0;
    } else if (imp && strncmp(imp, "__imp_", 6) == 0) {
        exp = imp + 6;
        is_data = 1;
    }
    if (want_is_imp_alias) is_data = 1;
    int hint = 0;
    char *from6 = idata6_name(find_sec(sec, nsec, ".idata$6"), &hint);
    if (from6 && from6[0]) exp = from6;
    char *dll = NULL;
    if (head) dll = dll_via_head(a, head);
    if (!dll) {
        /* Last resort: a .idata$7 string in this member. */
        dll = idata7_string(find_sec(sec, nsec, ".idata$7"));
    }
    if (dll && exp && exp[0])
        add_import(imps, nimps, dll, exp, is_data, 0, hint);
    free(dll);
    if (from6 != exp) free(from6);
    else free(from6);
    (void)want;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Symbol sets and the lazy scan                                       */
/* ------------------------------------------------------------------ */

static int set_has(char **s, size_t n, const char *name) {
    for (size_t i = 0; i < n; i++)
        if (strcmp(s[i], name) == 0) return 1;
    return 0;
}

static void set_add(char ***s, size_t *n, const char *name) {
    if (!name || !*name || set_has(*s, *n, name)) return;
    *s = xrealloc(*s, (*n + 1) * sizeof(char *));
    (*s)[(*n)++] = xstrdup(name);
}

static void set_free(char **s, size_t n) {
    for (size_t i = 0; i < n; i++) free(s[i]);
    free(s);
}

static void harvest_defs(const EmitModule *m, char ***def, size_t *ndef,
                         char ***und, size_t *nund) {
    for (size_t i = 0; i < m->num_syms; i++) {
        const EmitSymbol *s = &m->syms[i];
        if (!s->name || s->binding == 0) continue;
        if (s->shndx == SECT_UNDEF)
            set_add(und, nund, s->name);
        else if (s->shndx != SHN_COMMON)
            set_add(def, ndef, s->name);
        else
            set_add(def, ndef, s->name);
    }
}

static int file_ok(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static char *join2(const char *dir, const char *base) {
    size_t nd = strlen(dir), nb = strlen(base);
    int slash = !(nd && (dir[nd - 1] == '/' || dir[nd - 1] == '\\'));
    char *p = malloc(nd + (size_t)slash + nb + 1);
    if (!p) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    memcpy(p, dir, nd);
    if (slash) p[nd++] = '\\';
    memcpy(p + nd, base, nb + 1);
    return p;
}

static int explicit_lib_path(const char *s) {
    if (!s || !s[0] || s[0] == ':') return 0;
    if (strchr(s, '/') || strchr(s, '\\')) return 1;
    size_t n = strlen(s);
    if (n > 6 && strcmp(s + n - 6, ".dll.a") == 0) return 1;
    if (n > 2 && strcmp(s + n - 2, ".a") == 0) return 1;
    if (n > 4 && strcmp(s + n - 4, ".lib") == 0) return 1;
    if (n > 4 && strcmp(s + n - 4, ".dll") == 0) return 1;
    return 0;
}

static char *find_lib(const char *spec, const char **paths, size_t npaths) {
    if (explicit_lib_path(spec))
        return file_ok(spec) ? xstrdup(spec) : NULL;
    const char *exact = (spec[0] == ':') ? spec + 1 : NULL;
    const char *stem = exact ? NULL : spec;
    for (size_t d = 0; d < npaths; d++) {
        if (exact) {
            char *p = join2(paths[d], exact);
            if (file_ok(p)) return p;
            free(p);
            continue;
        }
        static const char *pat[] = {
            "lib%s.dll.a", "%s.dll.a", "lib%s.a", "%s.lib",
            "lib%s.lib", "%s.dll", "lib%s.dll"
        };
        for (size_t i = 0; i < sizeof pat / sizeof pat[0]; i++) {
            char base[512];
            snprintf(base, sizeof base, pat[i], stem);
            char *p = join2(paths[d], base);
            if (file_ok(p)) return p;
            free(p);
        }
    }
    /* Also accept a stem that is already a filename in the cwd. */
    if (exact && file_ok(exact)) return xstrdup(exact);
    return NULL;
}

static void note_import_syms(PeImport *im, char ***def, size_t *ndef) {
    set_add(def, ndef, im->name);
    char buf[512];
    snprintf(buf, sizeof buf, "__imp_%s", im->name);
    set_add(def, ndef, buf);
}

void pe_imports_free(PeImport *imps, size_t n) {
    if (!imps) return;
    for (size_t i = 0; i < n; i++) {
        free(imps[i].dll);
        free(imps[i].name);
    }
    free(imps);
}

int pe_pull_libraries(EmitModule **mods, size_t nmods,
                      const char **needed, size_t nneeded,
                      const char **lib_paths, size_t npaths,
                      EmitModule ***extra, size_t *nextra,
                      PeImport **imports, size_t *nimports) {
    *extra = NULL;
    *nextra = 0;
    *imports = NULL;
    *nimports = 0;
    if (!nneeded) return 0;

    char **def = NULL, **und = NULL;
    size_t ndef = 0, nund = 0;
    for (size_t i = 0; i < nmods; i++)
        harvest_defs(mods[i], &def, &ndef, &und, &nund);

    Archive *ars = calloc(nneeded ? nneeded : 1, sizeof(Archive));
    size_t nars = 0;
    int rc = 0;
    for (size_t i = 0; i < nneeded; i++) {
        char *path = find_lib(needed[i], lib_paths, npaths);
        if (!path) {
            /* libc/libm/libpthread are the fakecc runtime on Windows.  -lc
             * from a torture port is not a separate import library. */
            const char *stem = needed[i][0] == ':' ? needed[i] + 1 : needed[i];
            if (strcmp(stem, "c") == 0 || strcmp(stem, "m") == 0
                || strcmp(stem, "pthread") == 0 || strcmp(stem, "dl") == 0
                || strcmp(stem, "rt") == 0)
                continue;
            fprintf(stderr, "fakecc: cannot find -l library '%s'\n",
                    needed[i][0] == ':' ? needed[i] + 1 : needed[i]);
            rc = -1;
            break;
        }
        if (archive_load(&ars[nars], path) != 0) {
            free(path);
            archive_free(&ars[nars]);
            rc = -1;
            break;
        }
        free(path);
        /* A DLL's whole export set is the index. */
        if (ars[nars].is_dll) {
            for (size_t k = 0; k < ars[nars].n_dll_imps; k++)
                arch_add_sym(&ars[nars], ars[nars].dll_imps[k].name, (int)k);
        } else if (ars[nars].nsyms == 0) {
            /* No linker index: scan member symbol tables once. */
            for (size_t mi = 0; mi < ars[nars].nmem; mi++) {
                const char *nm = ars[nars].mem[mi].name;
                if (!nm || strcmp(nm, "/") == 0 || strcmp(nm, "//") == 0
                    || strcmp(nm, "/SYM64/") == 0)
                    continue;
                const unsigned char *p = ars[nars].data + ars[nars].mem[mi].hdr + 60;
                uint32_t sz = ars[nars].mem[mi].size;
                if (sz >= 20 && ru16(p) == 0 && ru16(p + 2) == 0xFFFF) {
                    const char *sym = (const char *)p + 20;
                    arch_add_sym(&ars[nars], sym, (int)mi);
                    continue;
                }
                CSec sec[16];
                CSym sym[64];
                int nsec = 0, nsym = 0;
                if (parse_coff_min(p, sz, sec, &nsec, sym, &nsym) != 0) continue;
                for (int s = 0; s < nsym; s++) {
                    if (sym[s].sclass == IMAGE_SYM_CLASS_EXTERNAL && sym[s].sec > 0
                        && sym[s].name[0] && sym[s].name[0] != '.')
                        arch_add_sym(&ars[nars], sym[s].name, (int)mi);
                }
            }
        }
        nars++;
    }

    int guard = 0;
    while (rc == 0 && guard++ < 10000) {
        int progress = 0;
        for (size_t u = 0; u < nund; u++) {
            const char *name = und[u];
            if (set_has(def, ndef, name)) continue;
            if (strcmp(name, "__fakecc_win_syscall") == 0) {
                set_add(&def, &ndef, name);
                continue;
            }
            for (size_t ai = 0; ai < nars; ai++) {
                Archive *a = &ars[ai];
                int alias = 0;
                int mi = arch_find(a, name);
                char aliasbuf[512];
                if (mi < 0 && strncmp(name, "__imp_", 6) != 0) {
                    snprintf(aliasbuf, sizeof aliasbuf, "__imp_%s", name);
                    mi = arch_find(a, aliasbuf);
                    if (mi >= 0) alias = 1;
                }
                if (mi < 0) continue;
                if (a->is_dll) {
                    PeImport *src = &a->dll_imps[mi];
                    if (add_import(imports, nimports, src->dll,
                                   alias ? name : src->name,
                                   alias ? 1 : src->is_data,
                                   src->ordinal, src->hint)) {
                        note_import_syms(&(*imports)[*nimports - 1], &def, &ndef);
                        if (alias) set_add(&def, &ndef, name);
                        progress = 1;
                    } else {
                        set_add(&def, &ndef, name);
                    }
                    break;
                }
                if (a->taken[mi]) {
                    set_add(&def, &ndef, name);
                    progress = 1;
                    break;
                }
                size_t before = *nimports;
                int is_imp = take_import_member(a, mi, name, alias, imports, nimports,
                                                &def, &ndef);
                a->taken[mi] = 1;
                if (is_imp) {
                    if (*nimports > before)
                        note_import_syms(&(*imports)[*nimports - 1], &def, &ndef);
                    set_add(&def, &ndef, name);
                    progress = 1;
                    break;
                }
                EmitModule *em = calloc(1, sizeof *em);
                if (!em) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
                const unsigned char *p = a->data + a->mem[mi].hdr + 60;
                if (coff_read_object_mem(p, a->mem[mi].size,
                                         a->mem[mi].name ? a->mem[mi].name : a->path,
                                         em) != 0) {
                    free(em);
                    rc = -1;
                    break;
                }
                em->is_user_tu = 0;
                *extra = xrealloc(*extra, (*nextra + 1) * sizeof(EmitModule *));
                (*extra)[(*nextra)++] = em;
                size_t u0 = nund;
                harvest_defs(em, &def, &ndef, &und, &nund);
                (void)u0;
                progress = 1;
                break;
            }
            if (rc) break;
        }
        if (!progress) break;
    }

    for (size_t i = 0; i < nars; i++) archive_free(&ars[i]);
    free(ars);
    set_free(def, ndef);
    set_free(und, nund);
    return rc;
}

/* ------------------------------------------------------------------ */
/* Import-library writer                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[8][9];
    const void *raw[8];
    uint32_t rawsz[8];
    uint32_t chars[8];
    int nsec;
    const char *sym[16];
    int16_t symsec[16];
    uint16_t symtype[16];
    uint8_t symcls[16];
    int nsym;
    int relsec[12];
    uint32_t reloff[12];
    int relsym[12];
    uint16_t reltype[12];
    int nrel;
} MiniCoff;

static void mini_emit(const MiniCoff *o, Buffer *out) {
    buffer_init(out);
    /* Lay out raw + relocs after headers so offsets are known. */
    uint32_t hdr = 20u + (uint32_t)o->nsec * 40u;
    uint32_t raw_at[8], rel_at[8], nrelsec[8];
    memset(raw_at, 0, sizeof raw_at);
    memset(rel_at, 0, sizeof rel_at);
    memset(nrelsec, 0, sizeof nrelsec);
    uint32_t cursor = hdr;
    for (int i = 0; i < o->nsec; i++) {
        if (!o->rawsz[i]) continue;
        cursor = (cursor + 3u) & ~3u;
        raw_at[i] = cursor;
        cursor += o->rawsz[i];
    }
    for (int r = 0; r < o->nrel; r++) nrelsec[o->relsec[r]]++;
    for (int i = 0; i < o->nsec; i++) {
        if (!nrelsec[i]) continue;
        cursor = (cursor + 3u) & ~3u;
        rel_at[i] = cursor;
        cursor += nrelsec[i] * 10u;
    }
    /* String table for long symbol names. */
    Buffer strs;
    buffer_init(&strs);
    wu32(&strs, 4);
    uint32_t stroff[16];
    memset(stroff, 0, sizeof stroff);
    for (int i = 0; i < o->nsym; i++) {
        if (strlen(o->sym[i]) <= 8) continue;
        stroff[i] = (uint32_t)strs.len;
        buffer_append(&strs, o->sym[i], strlen(o->sym[i]) + 1);
    }
    uint32_t strsz = (uint32_t)strs.len;
    memcpy(strs.data, &strsz, 4);
    uint32_t sym_at = cursor;

    wu16(out, IMAGE_FILE_MACHINE_AMD64);
    wu16(out, (uint16_t)o->nsec);
    wu32(out, 0);
    wu32(out, sym_at);
    wu32(out, (uint32_t)o->nsym);
    wu16(out, 0);
    wu16(out, 0);
    for (int i = 0; i < o->nsec; i++) {
        char nm[8];
        memset(nm, 0, 8);
        memcpy(nm, o->name[i], strlen(o->name[i]) < 8 ? strlen(o->name[i]) : 8);
        buffer_append(out, nm, 8);
        wu32(out, o->rawsz[i]);
        wu32(out, 0);
        wu32(out, o->rawsz[i]);
        wu32(out, raw_at[i]);
        wu32(out, rel_at[i]);
        wu32(out, 0);
        wu16(out, (uint16_t)nrelsec[i]);
        wu16(out, 0);
        wu32(out, o->chars[i]);
    }
    for (int i = 0; i < o->nsec; i++) {
        if (!o->rawsz[i]) continue;
        while (out->len < raw_at[i]) {
            char z = 0;
            buffer_append(out, &z, 1);
        }
        buffer_append(out, (const char *)o->raw[i], o->rawsz[i]);
    }
    for (int i = 0; i < o->nsec; i++) {
        if (!nrelsec[i]) continue;
        while (out->len < rel_at[i]) {
            char z = 0;
            buffer_append(out, &z, 1);
        }
        for (int r = 0; r < o->nrel; r++) {
            if (o->relsec[r] != i) continue;
            wu32(out, o->reloff[r]);
            wu32(out, (uint32_t)o->relsym[r]);
            wu16(out, o->reltype[r]);
        }
    }
    while (out->len < sym_at) {
        char z = 0;
        buffer_append(out, &z, 1);
    }
    for (int i = 0; i < o->nsym; i++) {
        char nm[8];
        memset(nm, 0, 8);
        if (strlen(o->sym[i]) <= 8) {
            memcpy(nm, o->sym[i], strlen(o->sym[i]));
        } else {
            memcpy(nm + 4, &stroff[i], 4);
        }
        buffer_append(out, nm, 8);
        wu32(out, 0);
        wu16(out, (uint16_t)o->symsec[i]);
        wu16(out, o->symtype[i]);
        char cls = (char)o->symcls[i];
        char aux = 0;
        buffer_append(out, &cls, 1);
        buffer_append(out, &aux, 1);
    }
    buffer_append(out, strs.data, strs.len);
    buffer_free(&strs);
}

static void mini_sec(MiniCoff *o, const char *name, const void *raw, uint32_t sz,
                     uint32_t chars) {
    int i = o->nsec++;
    snprintf(o->name[i], sizeof o->name[i], "%s", name);
    o->raw[i] = raw;
    o->rawsz[i] = sz;
    o->chars[i] = chars;
}

static void mini_sym(MiniCoff *o, const char *name, int sec, uint8_t cls) {
    int i = o->nsym++;
    o->sym[i] = name;
    o->symsec[i] = (int16_t)sec;
    o->symtype[i] = 0;
    o->symcls[i] = cls;
}

static void mini_rel(MiniCoff *o, int sec, uint32_t off, int sym, uint16_t ty) {
    int i = o->nrel++;
    o->relsec[i] = sec;
    o->reloff[i] = off;
    o->relsym[i] = sym;
    o->reltype[i] = ty;
}

#define SCN_R  (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_ALIGN_4BYTES)
#define SCN_X  (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_ALIGN_4BYTES)

static void sanitize(char *dst, size_t n, const char *dll) {
    size_t j = 0;
    for (size_t i = 0; dll[i] && j + 1 < n; i++) {
        unsigned char c = (unsigned char)dll[i];
        if (c == '/' || c == '\\') { j = 0; continue; }
        dst[j++] = isalnum(c) ? (char)c : '_';
    }
    dst[j] = 0;
    if (!j) snprintf(dst, n, "dll");
}

static void ar_append_member(Buffer *file, const char *name, const Buffer *payload) {
    char hdr[60];
    memset(hdr, ' ', 60);
    size_t nl = strlen(name);
    if (nl > 16) nl = 16;
    memcpy(hdr, name, nl);
    memcpy(hdr + 16, "0           ", 12);
    memcpy(hdr + 28, "0     ", 6);
    memcpy(hdr + 34, "0     ", 6);
    memcpy(hdr + 40, "0       ", 8);
    char sz[16];
    snprintf(sz, sizeof sz, "%-10u", (unsigned)payload->len);
    memcpy(hdr + 48, sz, 10);
    hdr[58] = '`';
    hdr[59] = '\n';
    buffer_append(file, hdr, 60);
    buffer_append(file, payload->data, payload->len);
    if (payload->len & 1) {
        char pad = '\n';
        buffer_append(file, &pad, 1);
    }
}

static char *basename_dll(const char *path) {
    const char *base = path ? path : "out.dll";
    for (const char *p = base; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    size_t n = strlen(base);
    if (n > 4 && icmp(base + n - 4, ".dll") == 0)
        n -= 4;
    /* libfoo -> foo for the import-lib stem, so we don't write liblibfoo. */
    const char *stem = base;
    size_t sn = n;
    if (sn > 3 && strncmp(stem, "lib", 3) == 0) {
        stem += 3;
        sn -= 3;
    }
    char *s = malloc(sn + 1);
    if (!s) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    memcpy(s, stem, sn);
    s[sn] = 0;
    return s;
}

static char *parent_dir(const char *path) {
    const char *slash = NULL;
    for (const char *p = path; p && *p; p++)
        if (*p == '/' || *p == '\\') slash = p;
    if (!slash) return xstrdup(".");
    size_t n = (size_t)(slash - path);
    if (!n) return xstrdup("\\");
    char *d = malloc(n + 1);
    if (!d) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    memcpy(d, path, n);
    d[n] = 0;
    return d;
}

static int write_gnu_implib(const char *path, const char *dll,
                            const PeImplibSym *syms, size_t nsyms) {
    char tag[128], head_sym[160], iname[160];
    sanitize(tag, sizeof tag, dll);
    snprintf(head_sym, sizeof head_sym, "_head_%s", tag);
    snprintf(iname, sizeof iname, "%s_iname", tag);

    unsigned char z20[20], z8[8], z4[4];
    memset(z20, 0, sizeof z20);
    memset(z8, 0, sizeof z8);
    memset(z4, 0, sizeof z4);
    unsigned char thunk[8] = { 0xFF, 0x25, 0, 0, 0, 0, 0x90, 0x90 };

    char **hintbuf = calloc(nsyms ? nsyms : 1, sizeof(char *));
    Buffer *payload = calloc(nsyms + 2, sizeof(Buffer));
    char (*mname)[16] = calloc(nsyms + 2, 16);
    if (!hintbuf || !payload || !mname) {
        fprintf(stderr, "fakecc: OOM\n");
        exit(1);
    }

    /* Head: h.o sorts before sNNNN.o and t.o. */
    {
        MiniCoff o;
        memset(&o, 0, sizeof o);
        mini_sec(&o, ".idata$2", z20, 20, SCN_R);
        mini_sec(&o, ".idata$5", NULL, 0, SCN_R);
        mini_sec(&o, ".idata$4", NULL, 0, SCN_R);
        mini_sym(&o, ".idata$2", 1, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$5", 2, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$4", 3, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, head_sym, 1, IMAGE_SYM_CLASS_EXTERNAL);
        mini_sym(&o, iname, 0, IMAGE_SYM_CLASS_EXTERNAL);
        mini_rel(&o, 0, 0, 2, IMAGE_REL_AMD64_ADDR32NB);
        mini_rel(&o, 0, 12, 4, IMAGE_REL_AMD64_ADDR32NB);
        mini_rel(&o, 0, 16, 1, IMAGE_REL_AMD64_ADDR32NB);
        mini_emit(&o, &payload[0]);
        snprintf(mname[0], 16, "h.o");
    }
    for (size_t i = 0; i < nsyms; i++) {
        size_t nl = strlen(syms[i].name);
        uint32_t hs = (uint32_t)(2 + nl + 1);
        if (hs & 1) hs++;
        hintbuf[i] = calloc(1, hs);
        memcpy(hintbuf[i] + 2, syms[i].name, nl + 1);
        char imp[300], nm[300];
        snprintf(imp, sizeof imp, "__imp_%s", syms[i].name);
        snprintf(nm, sizeof nm, "__nm_%s", syms[i].name);
        MiniCoff o;
        memset(&o, 0, sizeof o);
        int code = !syms[i].is_data;
        mini_sec(&o, ".text", code ? thunk : NULL, code ? 8 : 0, SCN_X);
        mini_sec(&o, ".idata$7", z4, 4, SCN_R);
        mini_sec(&o, ".idata$5", z8, 8, SCN_R);
        mini_sec(&o, ".idata$4", z8, 8, SCN_R);
        mini_sec(&o, ".idata$6", hintbuf[i], hs, SCN_R);
        mini_sym(&o, ".text", 1, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$7", 2, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$5", 3, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$4", 4, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$6", 5, IMAGE_SYM_CLASS_STATIC);
        if (code) mini_sym(&o, syms[i].name, 1, IMAGE_SYM_CLASS_EXTERNAL);
        mini_sym(&o, imp, 3, IMAGE_SYM_CLASS_EXTERNAL);
        if (!code) mini_sym(&o, nm, 5, IMAGE_SYM_CLASS_EXTERNAL);
        mini_sym(&o, head_sym, 0, IMAGE_SYM_CLASS_EXTERNAL);
        int head_si = o.nsym - 1;
        int idata6_si = 4;
        int idata5_si = 2;
        if (code) mini_rel(&o, 0, 2, idata5_si, IMAGE_REL_AMD64_REL32);
        mini_rel(&o, 1, 0, head_si, IMAGE_REL_AMD64_ADDR32NB);
        mini_rel(&o, 2, 0, idata6_si, IMAGE_REL_AMD64_ADDR32NB);
        mini_rel(&o, 3, 0, idata6_si, IMAGE_REL_AMD64_ADDR32NB);
        mini_emit(&o, &payload[1 + i]);
        snprintf(mname[1 + i], 16, "s%04u.o", (unsigned)i);
    }
    {
        size_t dl = strlen(dll);
        char *dname = calloc(1, dl + 1);
        memcpy(dname, dll, dl + 1);
        MiniCoff o;
        memset(&o, 0, sizeof o);
        mini_sec(&o, ".idata$4", z8, 8, SCN_R);
        mini_sec(&o, ".idata$5", z8, 8, SCN_R);
        mini_sec(&o, ".idata$7", dname, (uint32_t)dl + 1, SCN_R);
        mini_sym(&o, ".idata$4", 1, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$5", 2, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, ".idata$7", 3, IMAGE_SYM_CLASS_STATIC);
        mini_sym(&o, iname, 3, IMAGE_SYM_CLASS_EXTERNAL);
        mini_emit(&o, &payload[1 + nsyms]);
        snprintf(mname[1 + nsyms], 16, "t.o");
        /* dname is inside the payload copy; free after emit. */
        free(dname);
    }

    /* GNU symbol index, then members.  Offsets depend on the index size,
     * so build the index strings first and compute the header size. */
    Buffer symstr;
    buffer_init(&symstr);
    size_t nindex = 0;
    /* head, each public name, __imp_, optional __nm_, tail */
    nindex = 2;
    for (size_t i = 0; i < nsyms; i++) {
        nindex += syms[i].is_data ? 2 : 2;
    }
    char **inames = calloc(nindex, sizeof(char *));
    int *imem = calloc(nindex, sizeof(int));
    size_t ni = 0;
    inames[ni] = xstrdup(head_sym); imem[ni++] = 0;
    inames[ni] = xstrdup(iname); imem[ni++] = (int)(1 + nsyms);
    for (size_t i = 0; i < nsyms; i++) {
        char buf[300];
        if (!syms[i].is_data) {
            inames[ni] = xstrdup(syms[i].name);
            imem[ni++] = (int)(1 + i);
        }
        snprintf(buf, sizeof buf, "__imp_%s", syms[i].name);
        inames[ni] = xstrdup(buf);
        imem[ni++] = (int)(1 + i);
        if (syms[i].is_data) {
            snprintf(buf, sizeof buf, "__nm_%s", syms[i].name);
            inames[ni] = xstrdup(buf);
            imem[ni++] = (int)(1 + i);
        }
    }
    /* ni may be less than the conservative nindex if we miscounted; use ni. */
    for (size_t i = 0; i < ni; i++)
        buffer_append(&symstr, inames[i], strlen(inames[i]) + 1);
    uint32_t index_payload = 4u + (uint32_t)ni * 4u + (uint32_t)symstr.len;
    uint32_t index_even = index_payload + (index_payload & 1u);
    /* file: magic + symtab member + payloads */
    uint32_t cursor = 8u + 60u + index_even;
    uint32_t *moff = calloc(nsyms + 2, sizeof(uint32_t));
    for (size_t i = 0; i < nsyms + 2; i++) {
        moff[i] = cursor;
        uint32_t psz = (uint32_t)payload[i].len;
        cursor += 60u + psz + (psz & 1u);
    }
    /* GNU ar's symbol index is big-endian, same as the first MSVC linker
     * member.  A little-endian count makes ld reject the whole archive. */
    Buffer index;
    buffer_init(&index);
    {
        unsigned char b4[4];
        b4[0] = (unsigned char)((ni >> 24) & 0xff);
        b4[1] = (unsigned char)((ni >> 16) & 0xff);
        b4[2] = (unsigned char)((ni >> 8) & 0xff);
        b4[3] = (unsigned char)(ni & 0xff);
        buffer_append(&index, (const char *)b4, 4);
        for (size_t i = 0; i < ni; i++) {
            uint32_t off = moff[imem[i]];
            b4[0] = (unsigned char)((off >> 24) & 0xff);
            b4[1] = (unsigned char)((off >> 16) & 0xff);
            b4[2] = (unsigned char)((off >> 8) & 0xff);
            b4[3] = (unsigned char)(off & 0xff);
            buffer_append(&index, (const char *)b4, 4);
        }
    }
    buffer_append(&index, symstr.data, symstr.len);
    if (index.len & 1) {
        char pad = '\n';
        buffer_append(&index, &pad, 1);
    }

    Buffer file;
    buffer_init(&file);
    buffer_append(&file, AR_MAGIC, 8);
    ar_append_member(&file, "/", &index);
    for (size_t i = 0; i < nsyms + 2; i++)
        ar_append_member(&file, mname[i], &payload[i]);

    FILE *fp = fopen(path, "wb");
    int ok = fp && fwrite(file.data, 1, file.len, fp) == file.len;
    if (fp) fclose(fp);
    if (!ok) fprintf(stderr, "fakecc: cannot write import library '%s'\n", path);

    buffer_free(&file);
    buffer_free(&index);
    buffer_free(&symstr);
    for (size_t i = 0; i < nsyms + 2; i++) buffer_free(&payload[i]);
    for (size_t i = 0; i < nsyms; i++) free(hintbuf[i]);
    for (size_t i = 0; i < ni; i++) free(inames[i]);
    free(hintbuf); free(payload); free(mname);
    free(inames); free(imem); free(moff);
    return ok ? 0 : -1;
}

static int write_short_implib(const char *path, const char *dll,
                              const PeImplibSym *syms, size_t nsyms) {
    /* One short import object per published symbol.  Code exports are
     * indexed as both `name` and `__imp_name`; data exports only as
     * `__imp_name` (MSVC dllimport). */
    Buffer *payload = calloc(nsyms ? nsyms : 1, sizeof(Buffer));
    size_t nindex = 0;
    for (size_t i = 0; i < nsyms; i++) nindex += syms[i].is_data ? 1 : 2;
    char **inames = calloc(nindex ? nindex : 1, sizeof(char *));
    int *imem = calloc(nindex ? nindex : 1, sizeof(int));
    size_t ni = 0;
    for (size_t i = 0; i < nsyms; i++) {
        const char *sym = syms[i].name;
        size_t sl = strlen(sym), dl = strlen(dll);
        uint32_t sos = (uint32_t)(sl + 1 + dl + 1);
        unsigned char *obj = calloc(1, 20 + sos);
        obj[0] = 0; obj[1] = 0;
        obj[2] = 0xFF; obj[3] = 0xFF;
        obj[4] = 0; obj[5] = 0;
        obj[6] = 0x64; obj[7] = 0x86; /* AMD64 */
        memcpy(obj + 12, &sos, 4);
        uint16_t flags = (uint16_t)((1 << 2) | (syms[i].is_data ? 1 : 0));
        memcpy(obj + 18, &flags, 2);
        memcpy(obj + 20, sym, sl + 1);
        memcpy(obj + 20 + sl + 1, dll, dl + 1);
        buffer_init(&payload[i]);
        buffer_append(&payload[i], (const char *)obj, 20 + sos);
        free(obj);
        if (!syms[i].is_data) {
            inames[ni] = xstrdup(sym);
            imem[ni++] = (int)i;
        }
        char buf[300];
        snprintf(buf, sizeof buf, "__imp_%s", sym);
        inames[ni] = xstrdup(buf);
        imem[ni++] = (int)i;
    }

    Buffer symstr;
    buffer_init(&symstr);
    for (size_t i = 0; i < ni; i++)
        buffer_append(&symstr, inames[i], strlen(inames[i]) + 1);

    /* First linker member is big-endian; second is little-endian. */
    uint32_t be_payload = 4u + (uint32_t)ni * 4u + (uint32_t)symstr.len;
    uint32_t be_even = be_payload + (be_payload & 1);
    uint32_t le_payload = 4u + (uint32_t)nsyms * 4u + 4u + (uint32_t)ni * 2u
                        + (uint32_t)symstr.len;
    uint32_t le_even = le_payload + (le_payload & 1);
    uint32_t cursor = 8u + 60u + be_even + 60u + le_even;
    uint32_t *moff = calloc(nsyms ? nsyms : 1, sizeof(uint32_t));
    for (size_t i = 0; i < nsyms; i++) {
        moff[i] = cursor;
        uint32_t psz = (uint32_t)payload[i].len;
        cursor += 60u + psz + (psz & 1u);
    }

    Buffer be, le;
    buffer_init(&be);
    buffer_init(&le);
    unsigned char b4[4];
    b4[0] = (unsigned char)((ni >> 24) & 0xff);
    b4[1] = (unsigned char)((ni >> 16) & 0xff);
    b4[2] = (unsigned char)((ni >> 8) & 0xff);
    b4[3] = (unsigned char)(ni & 0xff);
    buffer_append(&be, (const char *)b4, 4);
    for (size_t i = 0; i < ni; i++) {
        uint32_t off = moff[imem[i]];
        b4[0] = (unsigned char)((off >> 24) & 0xff);
        b4[1] = (unsigned char)((off >> 16) & 0xff);
        b4[2] = (unsigned char)((off >> 8) & 0xff);
        b4[3] = (unsigned char)(off & 0xff);
        buffer_append(&be, (const char *)b4, 4);
    }
    buffer_append(&be, symstr.data, symstr.len);
    if (be.len & 1) {
        char pad = '\n';
        buffer_append(&be, &pad, 1);
    }

    wu32(&le, (uint32_t)nsyms);
    for (size_t i = 0; i < nsyms; i++) wu32(&le, moff[i]);
    wu32(&le, (uint32_t)ni);
    for (size_t i = 0; i < ni; i++) {
        uint16_t ix = (uint16_t)(imem[i] + 1);
        wu16(&le, ix);
    }
    buffer_append(&le, symstr.data, symstr.len);
    if (le.len & 1) {
        char pad = '\n';
        buffer_append(&le, &pad, 1);
    }

    Buffer file;
    buffer_init(&file);
    buffer_append(&file, AR_MAGIC, 8);
    ar_append_member(&file, "/", &be);
    ar_append_member(&file, "/", &le);
    for (size_t i = 0; i < nsyms; i++) {
        char nm[16];
        snprintf(nm, sizeof nm, "i%04u.o", (unsigned)i);
        ar_append_member(&file, nm, &payload[i]);
    }
    FILE *fp = fopen(path, "wb");
    int ok = fp && fwrite(file.data, 1, file.len, fp) == file.len;
    if (fp) fclose(fp);
    if (!ok) fprintf(stderr, "fakecc: cannot write import library '%s'\n", path);

    buffer_free(&file);
    buffer_free(&be);
    buffer_free(&le);
    buffer_free(&symstr);
    for (size_t i = 0; i < nsyms; i++) buffer_free(&payload[i]);
    for (size_t i = 0; i < ni; i++) free(inames[i]);
    free(payload); free(inames); free(imem); free(moff);
    return ok ? 0 : -1;
}

int pe_write_import_libs(const char *dll_path, const char *implib_path,
                         const char *dll_name,
                         const PeImplibSym *syms, size_t nsyms) {
    if (!dll_name || !dll_name[0]) {
        fprintf(stderr, "fakecc: DLL has no name for the import library\n");
        return -1;
    }
    char *stem = basename_dll(dll_path ? dll_path : dll_name);
    char *dir = parent_dir(implib_path ? implib_path : (dll_path ? dll_path : "."));
    char gnu_path[1024], msvc_path[1024];
    if (implib_path)
        snprintf(gnu_path, sizeof gnu_path, "%s", implib_path);
    else
        snprintf(gnu_path, sizeof gnu_path, "%s\\lib%s.dll.a", dir, stem);
    snprintf(msvc_path, sizeof msvc_path, "%s\\%s.lib", dir, stem);
    int rc = write_gnu_implib(gnu_path, dll_name, syms, nsyms);
    if (rc == 0) rc = write_short_implib(msvc_path, dll_name, syms, nsyms);
    free(stem);
    free(dir);
    return rc;
}
