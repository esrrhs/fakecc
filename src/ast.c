#include "fakecc/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Type — recursive helpers                                            */
/* ------------------------------------------------------------------ */

Type type_clone(Type t) {
    Type r = t;
    if (t.kind == TY_PTR && t.pointee) {
        /* struct types are shared — deep-copying them freezes the struct's
         * width at the time of the clone, which breaks self-referential structs
         * where the struct is still growing.  share the pointee instead. */
        if (t.pointee->kind == TY_STRUCT) {
            r.pointee = t.pointee;
        } else {
            r.pointee = malloc(sizeof(Type));
            if (!r.pointee) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            *r.pointee = type_clone(*t.pointee);
        }
    } else {
        r.pointee = NULL;
    }
    if (t.kind == TY_ARRAY && t.elem_type) {
        if (t.elem_type->kind == TY_STRUCT) {
            r.elem_type = t.elem_type;
        } else {
            r.elem_type = malloc(sizeof(Type));
            if (!r.elem_type) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            *r.elem_type = type_clone(*t.elem_type);
        }
    } else {
        r.elem_type = NULL;
    }
    r.tag = t.tag ? xstrdup(t.tag) : NULL;
    if (t.kind == TY_FUNC && t.func_ret) {
        if (t.func_ret->kind == TY_STRUCT) {
            r.func_ret = t.func_ret;
        } else {
            r.func_ret = malloc(sizeof(Type));
            if (!r.func_ret) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            *r.func_ret = type_clone(*t.func_ret);
        }
    } else {
        r.func_ret = NULL;
    }
    if (t.kind == TY_FUNC && t.func_nparams > 0 && t.func_params) {
        r.func_params = malloc(t.func_nparams * sizeof(Type));
        if (!r.func_params) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        for (int i = 0; i < t.func_nparams; i++)
            r.func_params[i] = type_clone(t.func_params[i]);
    } else {
        r.func_params = NULL;
    }
    return r;
}

void type_free(Type *t) {
    if (!t) return;
    /* struct types are shared (not owned by the pointer they're embedded in).
     * only free non-struct pointees / elem_types / func_rets. */
    if (t->pointee) {
        if (t->pointee->kind != TY_STRUCT) {
            type_free(t->pointee); free(t->pointee);
        }
        t->pointee = NULL;
    }
    if (t->elem_type) {
        if (t->elem_type->kind != TY_STRUCT) {
            type_free(t->elem_type); free(t->elem_type);
        }
        t->elem_type = NULL;
    }
    if (t->tag) { free(t->tag); t->tag = NULL; }
    if (t->func_ret) {
        if (t->func_ret->kind != TY_STRUCT) {
            type_free(t->func_ret); free(t->func_ret);
        }
        t->func_ret = NULL;
    }
    if (t->func_params) {
        for (int i = 0; i < t->func_nparams; i++) type_free(&t->func_params[i]);
        free(t->func_params); t->func_params = NULL;
    }
}

extern const StructRegistry *get_ir_structs(void);
extern const StructRegistry *get_parser_structs(void);

int type_size(Type t) {
    /* Walk array nesting through a pointer instead of recursing on the
     * by-value parameter.  Self-recursion here is rewritten by clang into a
     * loop that overwrites its own argument slot; because that slot is not
     * given a private copy, both this function and its caller then read a
     * mutated Type.  Never writing to `t` keeps the parameter intact. */
    const Type *p = &t;
    int count = 1;
    while (p->kind == TY_ARRAY && p->elem_type) {
        count *= p->length;
        p = p->elem_type;
    }
    switch (p->kind) {
    case TY_VOID:   return 0;
    case TY_INT:    return count * p->width;
    case TY_FLOAT:  return count * p->width;  /* 4 for float, 8 for double */
    case TY_PTR:    return count * 8;
    case TY_ARRAY:  return 0;   /* array without an element type — malformed */
    case TY_STRUCT: {
        if (p->tag) {
            const StructRegistry *reg = get_ir_structs();
            if (!reg) reg = get_parser_structs();
            if (reg) {
                const StructDef *sd = struct_registry_find_c(reg, p->tag);
                if (sd && sd->size > 0) return count * sd->size;
            }
        }
        return count * p->width;
    }
    case TY_FUNC:   return 0;        /* sizeof a function is undefined */
    }
    return 0;
}

Type type_make_ptr(Type pointee) {
    Type t; t.kind = TY_PTR; t.width = 8; t.is_unsigned = 1;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.elem_type = NULL; t.length = 0; t.tag = NULL;
    t.func_ret = NULL; t.func_params = NULL; t.func_nparams = 0;
    t.pointee = malloc(sizeof(Type));
    if (!t.pointee) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *t.pointee = type_clone(pointee);
    return t;
}

/* fixup all TY_STRUCT widths in a type tree that match the given tag.
 * this is called after a struct definition is complete so that self-referential
 * pointer members use the final struct size instead of a stale snapshot. */
static void type_fixup_struct_width(Type *t, const char *tag, int final_width) {
    if (!t || !tag) return;
    if (t->kind == TY_STRUCT && t->tag && strcmp(t->tag, tag) == 0) {
        t->width = final_width;
    }
    if (t->pointee) type_fixup_struct_width(t->pointee, tag, final_width);
    if (t->elem_type) type_fixup_struct_width(t->elem_type, tag, final_width);
    if (t->func_ret) type_fixup_struct_width(t->func_ret, tag, final_width);
    if (t->func_params) {
        for (int i = 0; i < t->func_nparams; i++)
            type_fixup_struct_width(&t->func_params[i], tag, final_width);
    }
}

void struct_def_fixup_self_types(StructDef *sd) {
    if (!sd || !sd->tag) return;
    for (int i = 0; i < sd->num_members; i++) {
        type_fixup_struct_width(&sd->members[i].type, sd->tag, sd->size);
    }
}

Type type_make_array(Type elem, int length) {
    Type t; t.kind = TY_ARRAY; t.width = elem.width;
    t.is_unsigned = elem.is_unsigned;
    t.is_const = elem.is_const; t.is_volatile = elem.is_volatile; t.is_restrict = elem.is_restrict;
    t.is_bool = 0; t.length = length;
    t.pointee = NULL; t.tag = NULL;
    t.func_ret = NULL; t.func_params = NULL; t.func_nparams = 0;
    t.elem_type = malloc(sizeof(Type));
    if (!t.elem_type) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *t.elem_type = type_clone(elem);
    return t;
}

Type type_make_struct(const char *tag, int size) {
    Type t; t.kind = TY_STRUCT; t.width = size; t.is_unsigned = 0;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = NULL; t.elem_type = NULL; t.length = 0;
    t.func_ret = NULL; t.func_params = NULL; t.func_nparams = 0;
    t.tag = xstrdup(tag);
    return t;
}

Type type_make_func(Type ret, Type * const *params, int nparams) {
    Type t; t.kind = TY_FUNC; t.width = 0; t.is_unsigned = 0; t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = NULL; t.elem_type = NULL; t.length = 0; t.tag = NULL;
    t.func_ret = malloc(sizeof(Type));
    if (!t.func_ret) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *t.func_ret = type_clone(ret);
    t.func_nparams = nparams;
    if (nparams > 0) {
        t.func_params = malloc(nparams * sizeof(Type));
        if (!t.func_params) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        for (int i = 0; i < nparams; i++)
            t.func_params[i] = type_clone(*params[i]);
    } else {
        t.func_params = NULL;
    }
    return t;
}

static int types_equal(Type a, Type b);  /* forward */

int type_funcs_equal(Type a, Type b) {
    if (a.kind != TY_FUNC || b.kind != TY_FUNC) return 0;
    if (!types_equal(*a.func_ret, *b.func_ret)) return 0;
    if (a.func_nparams != b.func_nparams) return 0;
    for (int i = 0; i < a.func_nparams; i++)
        if (!types_equal(a.func_params[i], b.func_params[i])) return 0;
    return 1;
}

static int types_equal(Type a, Type b) {
    if (a.kind != b.kind) return 0;
    switch (a.kind) {
    case TY_VOID: return 1;
    case TY_INT: return a.width == b.width && a.is_unsigned == b.is_unsigned;
    case TY_FLOAT: return a.width == b.width;  /* float vs double */
    case TY_PTR: return types_equal(*a.pointee, *b.pointee);
    case TY_ARRAY: return a.length == b.length && types_equal(*a.elem_type, *b.elem_type);
    case TY_STRUCT: return a.tag && b.tag && strcmp(a.tag, b.tag) == 0;
    case TY_FUNC: return type_funcs_equal(a, b);
    }
    return 0;
}

Type type_decay(Type t) {
    if (t.kind == TY_ARRAY) {
        Type r = type_make_ptr(*t.elem_type);
        return r;
    }
    return type_clone(t);
}

int type_is_ptr_or_array(Type t) {
    return t.kind == TY_PTR || t.kind == TY_ARRAY;
}

Type type_pointee_or_elem(Type t) {
    if (t.kind == TY_PTR)   return type_clone(*t.pointee);
    if (t.kind == TY_ARRAY) return type_clone(*t.elem_type);
    return type_default_int();  /* caller should have checked */
}

void expr_set_type(Expr *e, Type t) {
    if (!e) { type_free(&t); return; }
    type_free(&e->type);
    e->type = t;
}

/* ------------------------------------------------------------------ */
/* Struct registry                                                     */
/* ------------------------------------------------------------------ */

void struct_registry_init(StructRegistry *r) {
    r->data = NULL; r->len = 0; r->cap = 0;
}

void struct_registry_free(StructRegistry *r) {
    for (size_t i = 0; i < r->len; i++) {
        StructDef *sd = &r->data[i];
        free(sd->tag);
        for (int j = 0; j < sd->num_members; j++) {
            free(sd->members[j].name);
            type_free(&sd->members[j].type);
        }
        free(sd->members);
    }
    free(r->data);
    r->data = NULL; r->len = 0; r->cap = 0;
}

StructDef *struct_registry_add(StructRegistry *r, const char *tag, SourceLoc loc) {
    if (r->len >= r->cap) {
        size_t nc = r->cap ? r->cap * 2 : 4;
        r->data = realloc(r->data, nc * sizeof(StructDef));
        if (!r->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        r->cap = nc;
    }
    StructDef *sd = &r->data[r->len++];
    sd->tag = xstrdup(tag);
    sd->is_union = 0;
    sd->members = NULL; sd->num_members = 0; sd->cap_members = 0;
    sd->size = 0; sd->align = 1; sd->loc = loc;
    sd->bf_unit_type = 0; sd->bf_unit_used = 0; sd->bf_unit_offset = 0;
    sd->canonical_type = NULL;
    return sd;
}

StructDef *struct_registry_find(StructRegistry *r, const char *tag) {
    for (size_t i = 0; i < r->len; i++)
        if (strcmp(r->data[i].tag, tag) == 0) return &r->data[i];
    return NULL;
}

const StructDef *struct_registry_find_c(const StructRegistry *r, const char *tag) {
    return struct_registry_find((StructRegistry *)r, tag);
}

/* Round up x to a multiple of align. */
static int align_up(int x, int align) {
    if (align <= 1) return x;
    return (x + align - 1) & ~(align - 1);
}

/* Natural alignment of a type: 1/2/4/8 for scalars, elem's alignment for
 * arrays, max member alignment for structs. */
int type_align(Type t) {
    /* Pointer walk rather than self-recursion — see type_size(). */
    const Type *p = &t;
    while (p->kind == TY_ARRAY && p->elem_type) p = p->elem_type;
    switch (p->kind) {
    case TY_VOID:  return 1;    /* void has no size; alignment is a no-op */
    case TY_INT:   return p->width;
    case TY_FLOAT: return p->width;  /* float aligns to 4, double to 8 */
    case TY_PTR:   return 8;
    case TY_ARRAY: return 1;    /* array without an element type — malformed */
    case TY_STRUCT: {
        if (p->tag) {
            const StructRegistry *reg = get_ir_structs();
            if (!reg) reg = get_parser_structs();
            if (reg) {
                const StructDef *sd = struct_registry_find_c(reg, p->tag);
                if (sd && sd->align > 0) return sd->align;
            }
        }
        return 8;   /* fallback if incomplete/unknown */
    }
    case TY_FUNC:  return 1;    /* bare function has no size */
    }
    return 1;
}

/* Field class for SysV eightbyte merging (NO_CLASS = 0). */
enum { SV_NO = 0, SV_INT = 1, SV_SSE = 2, SV_MEM = 3 };

static int sysv_merge(int a, int b) {
    if (a == b) return a;
    if (a == SV_NO) return b;
    if (b == SV_NO) return a;
    if (a == SV_MEM || b == SV_MEM) return SV_MEM;
    if (a == SV_INT || b == SV_INT) return SV_INT;
    return SV_SSE;
}

static int sysv_field_class(Type t) {
    switch (t.kind) {
    case TY_INT:
    case TY_PTR:
    case TY_ARRAY:   /* decays; treated as pointer-sized when nested oddly */
        return SV_INT;
    case TY_FLOAT:
        if (t.width == 16) return SV_MEM; /* long double / X87 → MEMORY */
        return SV_SSE;
    case TY_STRUCT:
        return SV_MEM; /* nested handled by walking members of the outer def */
    default:
        return SV_MEM;
    }
}

int sysv_classify_agg(Type t, SysVRegClass cls[2]) {
    cls[0] = SYSV_CLS_INTEGER;
    cls[1] = SYSV_CLS_INTEGER;
    if (t.kind != TY_STRUCT || !t.tag) return 0;
    int sz = type_size(t);
    if (sz <= 0 || sz > 16) return 0;
    const StructRegistry *reg = get_ir_structs();
    /* During sema there may be no IR registry; fall back to size-only for
     * incomplete classification — treat as MEMORY to stay safe. */
    const StructDef *sd = NULL;
    if (reg) sd = struct_registry_find_c(reg, t.tag);
    if (!sd) {
        /* Size-only fallback used before ir_generate: ≤8 → 1 INTEGER,
         * ≤16 → 2 INTEGER, which matches all-integer structs (the common
         * case).  Floats/X87 are rare in early checks; ir_generate always
         * has the registry and re-classifies for real lowering. */
        int n = (sz + 7) / 8;
        cls[0] = SYSV_CLS_INTEGER;
        if (n > 1) cls[1] = SYSV_CLS_INTEGER;
        return n;
    }
    int eight[2] = { SV_NO, SV_NO };
    for (int mi = 0; mi < sd->num_members; mi++) {
        const StructMember *m = &sd->members[mi];
        int msz = m->bit_width > 0
                  ? (m->bit_width <= 8 ? 1 : m->bit_width <= 16 ? 2
                     : m->bit_width <= 32 ? 4 : 8)
                  : type_size(m->type);
        if (msz <= 0) continue;
        /* Bitfields share a storage unit starting at m->offset. */
        int start = m->offset;
        int end = start + msz;
        int fc = sysv_field_class(m->type);
        if (m->bit_width > 0) fc = SV_INT;
        if (fc == SV_MEM) return 0;
        for (int eb = 0; eb < 2; eb++) {
            int lo = eb * 8, hi = lo + 8;
            if (end <= lo || start >= hi) continue;
            eight[eb] = sysv_merge(eight[eb], fc);
            if (eight[eb] == SV_MEM) return 0;
        }
    }
    int n = (sz + 7) / 8;
    if (n < 1) n = 1;
    if (n > 2) return 0;
    for (int i = 0; i < n; i++) {
        int c = eight[i] == SV_NO ? SV_INT : eight[i];
        if (c == SV_MEM) return 0;
        cls[i] = (c == SV_SSE) ? SYSV_CLS_SSE : SYSV_CLS_INTEGER;
    }
    /* Post-merger: >2 eightbytes already rejected; X87 already → MEM. */
    return n;
}

void struct_def_push_member(StructDef *sd, const char *name, Type ty, int bit_width) {
    if (sd->num_members >= sd->cap_members) {
        int nc = sd->cap_members ? sd->cap_members * 2 : 4;
        sd->members = realloc(sd->members, nc * sizeof(StructMember));
        if (!sd->members) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        sd->cap_members = nc;
    }
    int a = type_align(ty);
    int sz = type_size(ty);
    /* Track the max member alignment for final struct alignment. */
    if (a > sd->align) sd->align = a;
    int off;
    if (sd->is_union) {
        /* Union members all start at offset 0; total size is the max. */
        off = 0;
    } else if (bit_width > 0 && ty.kind == TY_INT) {
        /* Bitfield packing: a run of adjacent `unsigned/int : N` bitfields of
         * the same byte-width unit packs into one storage unit.  The unit
         * size is the smallest of {1,2,4,8} bytes that holds all its bits
         * (we use the declared type width, e.g. unsigned → 4 bytes). */
        int unit_bits = sz * 8;
        if (sd->bf_unit_type == sz && sd->bf_unit_used + bit_width <= unit_bits) {
            /* Fits in the current open unit. */
            off = sd->bf_unit_offset;
        } else {
            /* Close any open unit and start a new one.  Track the raw byte
             * end of the closed unit; defer alignment padding to
             * struct_def_finish() so a following member can still pack into
             * any trailing gap. */
            if (sd->bf_unit_used > 0)
                sd->size = sd->bf_unit_offset + sd->bf_unit_type;
            sd->bf_unit_type = sz;
            sd->bf_unit_used = 0;
            sd->bf_unit_offset = align_up(sd->size, a);
            off = sd->bf_unit_offset;
        }
        sd->members[sd->num_members].bit_offset = sd->bf_unit_used;
        sd->bf_unit_used += bit_width;
        sd->members[sd->num_members].offset = off;
        sd->members[sd->num_members].bit_width = bit_width;
        /* Advance logical struct size to cover this unit if it extends past. */
        int unit_end = sd->bf_unit_offset + sz;
        if (unit_end > sd->size) sd->size = unit_end;
        sd->members[sd->num_members].name = xstrdup(name);
        sd->members[sd->num_members].type = type_clone(ty);
        sd->num_members++;
        return;
    } else {
        /* Normal (non-bitfield) member: close any open bitfield unit first.
         * Track the raw byte end of the closed unit; alignment padding is
         * deferred to struct_def_finish() so trailing members can pack. */
        if (sd->bf_unit_used > 0) {
            sd->size = sd->bf_unit_offset + sd->bf_unit_type;
            sd->bf_unit_type = 0; sd->bf_unit_used = 0;
        }
        off = align_up(sd->size, a);
    }
    sd->members[sd->num_members].name = xstrdup(name);
    sd->members[sd->num_members].type = type_clone(ty);
    sd->members[sd->num_members].offset = off;
    sd->members[sd->num_members].bit_width = bit_width;
    sd->members[sd->num_members].bit_offset = 0;
    sd->num_members++;
    if (sd->is_union) {
        /* Size grows to the largest member.  Final alignment is applied once
         * in struct_def_finish(), not here — padding after every member would
         * prevent later members from packing into the trailing gap. */
        if (sz > sd->size) sd->size = sz;
    } else {
        /* Track the raw byte end of the member.  Alignment padding to the
         * struct's natural boundary is deferred to struct_def_finish() so
         * that a following member can still pack into any trailing gap
         * (e.g. `struct { void *p; int a; int b; }` packs a,b at 8,12). */
        sd->size = off + sz;
    }
}

/* Finalize a struct/union definition: round the total size up to the struct's
 * natural alignment (max member alignment).  Call once after the last member
 * is pushed.  Applying this per-member would prematurely pad the struct and
 * break trailing-member packing. */
void struct_def_finish(StructDef *sd) {
    sd->size = align_up(sd->size, sd->align);
}

/* ------------------------------------------------------------------ */
/* Switch case helper                                                    */
/* ------------------------------------------------------------------ */

void switch_push_case(Stmt *s, int is_default, int value) {
    if (s->u.switch_s.num_cases >= s->u.switch_s.cap_cases) {
        int nc = s->u.switch_s.cap_cases ? s->u.switch_s.cap_cases * 2 : 4;
        s->u.switch_s.cases = realloc(s->u.switch_s.cases,
                                      nc * sizeof(SwitchCase));
        if (!s->u.switch_s.cases) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        s->u.switch_s.cap_cases = nc;
    }
    SwitchCase *c = &s->u.switch_s.cases[s->u.switch_s.num_cases++];
    c->is_default = is_default;
    c->value = value;
    stmt_array_init(&c->stmts);
}

/* ------------------------------------------------------------------ */
/* Enum registry                                                        */
/* ------------------------------------------------------------------ */

void enum_registry_init(EnumRegistry *r) {
    r->data = NULL; r->len = 0; r->cap = 0;
}

void enum_registry_free(EnumRegistry *r) {
    for (size_t i = 0; i < r->len; i++) {
        EnumDef *ed = &r->data[i];
        free(ed->tag);
        for (int j = 0; j < ed->num_constants; j++)
            free(ed->constants[j].name);
        free(ed->constants);
    }
    free(r->data);
    r->data = NULL; r->len = 0; r->cap = 0;
}

EnumDef *enum_registry_add(EnumRegistry *r, const char *tag, SourceLoc loc) {
    if (r->len >= r->cap) {
        size_t nc = r->cap ? r->cap * 2 : 4;
        r->data = realloc(r->data, nc * sizeof(EnumDef));
        if (!r->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        r->cap = nc;
    }
    EnumDef *ed = &r->data[r->len++];
    ed->tag = tag ? xstrdup(tag) : NULL;
    ed->constants = NULL; ed->num_constants = 0; ed->cap_constants = 0;
    ed->loc = loc;
    return ed;
}

EnumDef *enum_registry_find(EnumRegistry *r, const char *tag) {
    if (!tag) return NULL;
    for (size_t i = 0; i < r->len; i++)
        if (r->data[i].tag && strcmp(r->data[i].tag, tag) == 0) return &r->data[i];
    return NULL;
}

int enum_def_push_constant(EnumDef *ed, const char *name, int has_value,
                           int value, SourceLoc loc) {
    (void)loc;
    if (ed->num_constants >= ed->cap_constants) {
        int nc = ed->cap_constants ? ed->cap_constants * 2 : 4;
        ed->constants = realloc(ed->constants, nc * sizeof(EnumConstant));
        if (!ed->constants) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        ed->cap_constants = nc;
    }
    int assigned;
    if (has_value) {
        assigned = value;
    } else {
        assigned = (ed->num_constants > 0)
            ? ed->constants[ed->num_constants - 1].value + 1 : 0;
    }
    ed->constants[ed->num_constants].name = xstrdup(name);
    ed->constants[ed->num_constants].value = assigned;
    ed->num_constants++;
    return assigned;
}

const EnumConstant *enum_registry_find_constant(const EnumRegistry *r,
                                                const char *name) {
    for (size_t i = 0; i < r->len; i++) {
        const EnumDef *ed = &r->data[i];
        for (int j = 0; j < ed->num_constants; j++)
            if (strcmp(ed->constants[j].name, name) == 0)
                return &ed->constants[j];
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Typedef registry                                                     */
/* ------------------------------------------------------------------ */

void typedef_registry_init(TypedefRegistry *r) {
    r->data = NULL; r->len = 0; r->cap = 0;
}

void typedef_registry_free(TypedefRegistry *r) {
    for (size_t i = 0; i < r->len; i++) {
        free(r->data[i].name);
        type_free(&r->data[i].type);
    }
    free(r->data);
    r->data = NULL; r->len = 0; r->cap = 0;
}

TypedefEntry *typedef_registry_add(TypedefRegistry *r, const char *name, Type type) {
    if (r->len >= r->cap) {
        size_t nc = r->cap ? r->cap * 2 : 4;
        r->data = realloc(r->data, nc * sizeof(TypedefEntry));
        if (!r->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        r->cap = nc;
    }
    TypedefEntry *e = &r->data[r->len++];
    e->name = xstrdup(name);
    e->type = type; /* takes ownership */
    return e;
}

const Type *typedef_registry_find(const TypedefRegistry *r, const char *name) {
    for (size_t i = 0; i < r->len; i++)
        if (strcmp(r->data[i].name, name) == 0) return &r->data[i].type;
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Expr constructors & destructor                                       */
/* ------------------------------------------------------------------ */

Expr *expr_new_int(long long v, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_INT_LIT;
    e->loc = loc;
    e->type = type_default_int();
    memset(&e->va_arg_type, 0, sizeof(e->va_arg_type));
    e->u.int_val = v;
    return e;
}

Expr *expr_new_int_typed(long long v, int width, int is_unsigned, SourceLoc loc) {
    Expr *e = expr_new_int(v, loc);
    e->type = type_make_int(width, is_unsigned);
    return e;
}

Expr *expr_new_binop(BinOp op, Expr *l, Expr *r, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_BINOP;
    e->loc = loc;
    e->type = type_default_int();
    memset(&e->va_arg_type, 0, sizeof(e->va_arg_type));
    e->u.bin.op = op;
    e->u.bin.l = l;
    e->u.bin.r = r;
    return e;
}

Expr *expr_new_unary(UnaryOp op, Expr *operand, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_UNARY;
    e->loc = loc;
    e->type = type_default_int();
    memset(&e->va_arg_type, 0, sizeof(e->va_arg_type));
    e->u.un.op = op;
    e->u.un.operand = operand;
    return e;
}

Expr *expr_new_var(const char *name, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_VAR;
    e->loc = loc;
    e->type = type_default_int();
    memset(&e->va_arg_type, 0, sizeof(e->va_arg_type));
    e->u.var.name = xstrdup(name);
    e->u.var.pkg = NULL;
    return e;
}

Expr *expr_new_var_qual(const char *pkg, const char *name, SourceLoc loc) {
    Expr *e = expr_new_var(name, loc);
    e->u.var.pkg = xstrdup(pkg);
    return e;
}

Expr *expr_new_assign(Expr *lvalue, Expr *rvalue, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_ASSIGN;
    e->loc = loc;
    e->type = type_default_int();
    memset(&e->va_arg_type, 0, sizeof(e->va_arg_type));
    e->u.assign.lvalue = lvalue;
    e->u.assign.rvalue = rvalue;
    return e;
}

Expr *expr_new_call(Expr *callee, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_CALL;
    e->loc = loc;
    e->type = type_default_int();
    memset(&e->va_arg_type, 0, sizeof(e->va_arg_type));
    e->u.call.callee = callee;   /* takes ownership */
    e->u.call.args.data = NULL;
    e->u.call.args.len = 0;
    e->u.call.args.cap = 0;
    return e;
}

void expr_call_set_callee(Expr *e, Expr *callee) {
    if (!e || e->kind != EX_CALL) return;
    if (e->u.call.callee) expr_free(e->u.call.callee);
    e->u.call.callee = callee;   /* takes ownership */
}

Expr *expr_new_str(const char *bytes, int len, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    e->kind = EX_STR;
    e->loc = loc;
    /* type is set by sema: char[len+1] initially, decays to char* on use */
    e->type = type_default_int();
    e->u.str.bytes = malloc(len + 1);
    if (!e->u.str.bytes) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    memcpy(e->u.str.bytes, bytes, len);
    e->u.str.bytes[len] = '\0';
    e->u.str.len = len;
    return e;
}

static Expr *expr_alloc(ExprKind k, SourceLoc loc);  /* forward */

Expr *expr_new_float_lit(const char *text, int width, SourceLoc loc) {
    Expr *e = expr_alloc(EX_FLOAT_LIT, loc);
    e->u.float_text = xstrdup(text);
    /* type is set by sema, but stash width now so parser-level consumers work */
    e->type = type_make_float(width);
    return e;
}

void expr_call_push_arg(Expr *e, Expr *arg) {
    ExprArray *a = &e->u.call.args;
    if (a->len >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 4;
        a->data = realloc(a->data, a->cap * sizeof(Expr *));
        if (!a->data) { fprintf(stderr, "fakecc: out of memory\n"); exit(1); }
    }
    a->data[a->len++] = arg;
}

static Expr *expr_alloc(ExprKind k, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    e->kind = k; e->loc = loc; e->type = type_default_int();
    memset(&e->va_arg_type, 0, sizeof(e->va_arg_type));
    return e;
}

Expr *expr_new_addr(Expr *operand, SourceLoc loc) {
    Expr *e = expr_alloc(EX_ADDR, loc); e->u.addr.operand = operand; return e;
}
Expr *expr_new_deref(Expr *operand, SourceLoc loc) {
    Expr *e = expr_alloc(EX_DEREF, loc); e->u.deref.operand = operand; return e;
}
Expr *expr_new_index(Expr *array, Expr *index, SourceLoc loc) {
    Expr *e = expr_alloc(EX_INDEX, loc);
    e->u.idx.array = array; e->u.idx.index = index; return e;
}
Expr *expr_new_member(Expr *obj, const char *name, SourceLoc loc) {
    Expr *e = expr_alloc(EX_MEMBER, loc);
    e->u.member.obj = obj;
    e->u.member.name = xstrdup(name);
    return e;
}
Expr *expr_new_cast(Type target, Expr *operand, SourceLoc loc) {
    Expr *e = expr_alloc(EX_CAST, loc);
    e->u.cast.target = type_clone(target); e->u.cast.operand = operand; return e;
}
Expr *expr_new_sizeof_type(Type t, SourceLoc loc) {
    Expr *e = expr_alloc(EX_SIZEOF_TYPE, loc);
    e->u.sizeof_t.target = type_clone(t); return e;
}
Expr *expr_new_sizeof_expr(Expr *operand, SourceLoc loc) {
    Expr *e = expr_alloc(EX_SIZEOF_EXPR, loc);
    e->u.sizeof_e.operand = operand; return e;
}
Expr *expr_new_alignof_type(Type t, SourceLoc loc) {
    Expr *e = expr_alloc(EX_ALIGNOF_TYPE, loc);
    e->u.alignof_t.target = type_clone(t); return e;
}
Expr *expr_new_ternary(Expr *cond, Expr *then, Expr *else_, SourceLoc loc) {
    Expr *e = expr_alloc(EX_TERNARY, loc);
    e->u.tern.cond = cond; e->u.tern.then = then; e->u.tern.else_ = else_; return e;
}
Expr *expr_new_inc_dec(Expr *operand, int is_inc, int is_prefix, SourceLoc loc) {
    Expr *e = expr_alloc(EX_INC_DEC, loc);
    e->u.incdec.operand = operand; e->u.incdec.is_inc = is_inc;
    e->u.incdec.is_prefix = is_prefix; return e;
}
Expr *expr_new_compound_assign(Expr *lvalue, Expr *rvalue, BinOp op, SourceLoc loc) {
    Expr *e = expr_alloc(EX_COMPOUND_ASSIGN, loc);
    e->u.comp.lvalue = lvalue; e->u.comp.rvalue = rvalue; e->u.comp.op = op; return e;
}
Expr *expr_new_comma(Expr *l, Expr *r, SourceLoc loc) {
    Expr *e = expr_alloc(EX_COMMA, loc);
    e->u.comma.lhs = l; e->u.comma.rhs = r; return e;
}
Expr *expr_new_init_list(Expr **elements, int num_elements, SourceLoc loc) {
    Expr *e = expr_alloc(EX_INIT_LIST, loc);
    e->u.init_list.elements = elements;
    e->u.init_list.num_elements = num_elements;
    e->u.init_list.desig_kind = malloc(num_elements * sizeof(int));
    e->u.init_list.desig_index = malloc(num_elements * sizeof(int));
    e->u.init_list.desig_member = calloc(num_elements, sizeof(char *));
    for (int i = 0; i < num_elements; i++) {
        e->u.init_list.desig_kind[i] = -1;
        e->u.init_list.desig_index[i] = -1;
    }
    return e;
}

Expr *expr_new_compound_literal(Type target_type, Expr *init, SourceLoc loc) {
    Expr *e = expr_alloc(EX_COMPOUND_LITERAL, loc);
    e->u.compound.target_type = type_clone(target_type); /* own a copy so the caller may type_free(&ty) */
    e->u.compound.init = init;
    return e;
}

Expr *expr_new_stmt_expr(StmtArray *stmts, SourceLoc loc) {
    Expr *e = expr_alloc(EX_STMT_EXPR, loc);
    e->u.stmt_expr.stmts = malloc(sizeof(StmtArray));
    if (!e->u.stmt_expr.stmts) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *e->u.stmt_expr.stmts = *stmts;
    return e;
}

Expr *expr_new_label_addr(const char *label, SourceLoc loc) {
    Expr *e = expr_alloc(EX_LABEL_ADDR, loc);
    e->u.label_addr.label = xstrdup(label);
    return e;
}

void expr_free(Expr *e) {
    if (!e) return;
    switch (e->kind) {
    case EX_INT_LIT:
        break;
    case EX_BINOP:
        expr_free(e->u.bin.l);
        expr_free(e->u.bin.r);
        break;
    case EX_UNARY:
        expr_free(e->u.un.operand);
        break;
    case EX_VAR:
        free(e->u.var.name);
        free(e->u.var.pkg);
        break;
    case EX_ASSIGN:
        expr_free(e->u.assign.lvalue);
        expr_free(e->u.assign.rvalue);
        break;
    case EX_CALL:
        expr_free(e->u.call.callee);
        for (size_t i = 0; i < e->u.call.args.len; i++)
            expr_free(e->u.call.args.data[i]);
        free(e->u.call.args.data);
        type_free(&e->va_arg_type);
        break;
    case EX_STR:
        free(e->u.str.bytes);
        break;
    case EX_FLOAT_LIT:
        free(e->u.float_text);
        break;
    case EX_ADDR:  expr_free(e->u.addr.operand); break;
    case EX_DEREF: expr_free(e->u.deref.operand); break;
    case EX_INDEX:
        expr_free(e->u.idx.array); expr_free(e->u.idx.index); break;
    case EX_MEMBER:
        expr_free(e->u.member.obj);
        free(e->u.member.name);
        break;
    case EX_CAST:
        type_free(&e->u.cast.target);
        expr_free(e->u.cast.operand);
        break;
    case EX_SIZEOF_TYPE:
        type_free(&e->u.sizeof_t.target); break;
    case EX_SIZEOF_EXPR:
        expr_free(e->u.sizeof_e.operand); break;
    case EX_TERNARY:
        expr_free(e->u.tern.cond);
        expr_free(e->u.tern.then);
        expr_free(e->u.tern.else_);
        break;
    case EX_INC_DEC:
        expr_free(e->u.incdec.operand);
        break;
    case EX_COMPOUND_ASSIGN:
        expr_free(e->u.comp.lvalue);
        expr_free(e->u.comp.rvalue);
        break;
    case EX_COMMA:
        expr_free(e->u.comma.lhs);
        expr_free(e->u.comma.rhs);
        break;
    case EX_INIT_LIST: {
        for (int i = 0; i < e->u.init_list.num_elements; i++)
            expr_free(e->u.init_list.elements[i]);
        free(e->u.init_list.elements);
        if (e->u.init_list.desig_member) {
            for (int i = 0; i < e->u.init_list.num_elements; i++)
                free(e->u.init_list.desig_member[i]);
        }
        free(e->u.init_list.desig_kind);
        free(e->u.init_list.desig_index);
        free(e->u.init_list.desig_member);
        break;
    }
    case EX_COMPOUND_LITERAL:
        type_free(&e->u.compound.target_type);
        expr_free(e->u.compound.init);
        break;
    case EX_ALIGNOF_TYPE:
        type_free(&e->u.alignof_t.target);
        break;
    case EX_STMT_EXPR:
        if (e->u.stmt_expr.stmts) {
            stmt_array_free(e->u.stmt_expr.stmts);
            free(e->u.stmt_expr.stmts);
        }
        break;
    case EX_LABEL_ADDR:
        free(e->u.label_addr.label);
        break;
    }
    type_free(&e->type);
    free(e);
}

/* ------------------------------------------------------------------ */
/* Stmt lifetime                                                       */
/* ------------------------------------------------------------------ */

void stmt_free(Stmt *s) {
    if (!s) return;
    switch (s->kind) {
    case ST_DECL:
        free(s->u.decl.name);
        type_free(&s->u.decl.type);
        expr_free(s->u.decl.init);
        break;
    case ST_EXPR:
        expr_free(s->u.expr);
        break;
    case ST_RETURN:
        expr_free(s->u.value);
        break;
    case ST_IF:
        expr_free(s->u.if_s.cond);
        stmt_free_ptr(s->u.if_s.then_s);
        stmt_free_ptr(s->u.if_s.else_s);
        break;
    case ST_WHILE:
        expr_free(s->u.while_s.cond);
        stmt_free_ptr(s->u.while_s.body);
        break;
    case ST_DO_WHILE:
        expr_free(s->u.do_s.cond);
        stmt_free_ptr(s->u.do_s.body);
        break;
    case ST_GOTO:
        free(s->u.goto_s.target);
        expr_free(s->u.goto_s.target_expr);
        break;
    case ST_LABEL:
        free(s->u.label_s.name);
        stmt_free_ptr(s->u.label_s.stmt);
        break;
    case ST_SWITCH:
        expr_free(s->u.switch_s.cond);
        for (int i = 0; i < s->u.switch_s.num_cases; i++)
            stmt_array_free(&s->u.switch_s.cases[i].stmts);
        free(s->u.switch_s.cases);
        break;
    case ST_FOR:
        stmt_free_ptr(s->u.for_s.init);
        expr_free(s->u.for_s.cond);
        expr_free(s->u.for_s.step);
        stmt_free_ptr(s->u.for_s.body);
        break;
    case ST_BREAK:
    case ST_CONTINUE:
        break;
    case ST_BLOCK:
        stmt_array_free(&s->u.block);
        break;
    }
}

Stmt *stmt_alloc(void) {
    Stmt *s = malloc(sizeof(Stmt));
    if (!s) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    return s;
}

void stmt_free_ptr(Stmt *s) {
    if (!s) return;
    stmt_free(s);
    free(s);
}

void stmt_array_init(StmtArray *a) {
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

void stmt_array_push(StmtArray *a, Stmt s) {
    if (a->len >= a->cap) {
        size_t new_cap = a->cap ? a->cap * 2 : 8;
        a->data = realloc(a->data, new_cap * sizeof(Stmt));
        if (!a->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        a->cap = new_cap;
    }
    a->data[a->len++] = s;
}

void stmt_array_free(StmtArray *a) {
    for (size_t i = 0; i < a->len; i++) {
        stmt_free(&a->data[i]);
    }
    free(a->data);
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

/* ------------------------------------------------------------------ */
/* TranslationUnit lifetime                                            */
/* ------------------------------------------------------------------ */

void import_array_init(ImportArray *a) {
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

void import_array_push(ImportArray *a, const char *name, SourceLoc loc) {
    if (a->len >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 4;
        a->data = realloc(a->data, a->cap * sizeof(ImportDecl));
        if (!a->data) { fprintf(stderr, "fakecc: out of memory\n"); exit(1); }
    }
    a->data[a->len].name = xstrdup(name);
    a->data[a->len].loc = loc;
    a->len++;
}

void import_array_free(ImportArray *a) {
    for (size_t i = 0; i < a->len; i++)
        free(a->data[i].name);
    free(a->data);
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

void tu_init(TranslationUnit *tu) {
    tu->package.name = NULL;
    tu->package.loc.file = NULL;
    tu->package.loc.line = 0;
    tu->package.loc.col = 0;
    import_array_init(&tu->imports);
    stmt_array_init(&tu->globals);
    tu->functions.data = NULL;
    tu->functions.len = 0;
    tu->functions.cap = 0;
    struct_registry_init(&tu->structs);
    enum_registry_init(&tu->enums);
    typedef_registry_init(&tu->typedefs);

    /* Predeclare the `va_list` type used by the va_start/va_arg/va_end
     * builtins. It mirrors the SysV AMD64 va_list layout so sizeof and field
     * layout resolve, even though user code never constructs one directly. */
    SourceLoc vloc = {0};
    StructDef *va = struct_registry_add(&tu->structs, "__va_list_tag", vloc);
    struct_def_push_member(va, "gp_offset", type_make_int(4, 1), 0);
    struct_def_push_member(va, "fp_offset", type_make_int(4, 1), 0);
    struct_def_push_member(va, "overflow_arg_area", type_make_ptr(type_make_void()), 0);
    struct_def_push_member(va, "reg_save_area", type_make_ptr(type_make_void()), 0);
    struct_def_finish(va);
    Type va_type = type_make_struct("__va_list_tag", va->size);
    typedef_registry_add(&tu->typedefs, "va_list", va_type);
}

void tu_free(TranslationUnit *tu) {
    free(tu->package.name);
    import_array_free(&tu->imports);
    stmt_array_free(&tu->globals);
    for (size_t i = 0; i < tu->functions.len; i++) {
        free(tu->functions.data[i].name);
        type_free(&tu->functions.data[i].ret_type);
        param_array_free(&tu->functions.data[i].params);
        stmt_array_free(&tu->functions.data[i].body);
    }
    free(tu->functions.data);
    struct_registry_free(&tu->structs);
    enum_registry_free(&tu->enums);
    typedef_registry_free(&tu->typedefs);
}

void param_array_init(ParamArray *a) {
    a->data = NULL; a->len = 0; a->cap = 0;
}

void param_array_push(ParamArray *a, const char *name, Type type, SourceLoc loc) {
    if (a->len >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 4;
        a->data = realloc(a->data, a->cap * sizeof(Param));
        if (!a->data) { fprintf(stderr, "fakecc: out of memory\n"); exit(1); }
    }
    a->data[a->len].name = xstrdup(name);
    a->data[a->len].type = type;
    a->data[a->len].loc = loc;
    a->len++;
}

void param_array_free(ParamArray *a) {
    for (size_t i = 0; i < a->len; i++) {
        free(a->data[i].name);
        type_free(&a->data[i].type);
    }
    free(a->data);
    a->data = NULL; a->len = 0; a->cap = 0;
}

/* ------------------------------------------------------------------ */
/* Compile-time integer constant folding                               */
/* ------------------------------------------------------------------ */

/* Try to fold `e` to a single integer constant.  Returns 1 and writes the
 * value to *out if `e` is an integer literal, a cast of one, or a unary/
 * binary operation on constant integer operands (e.g. `(1u << 14) - 1u`).
 * Returns 0 otherwise (non-constant, non-integer, or涉及 pointer values).
 * Used by sema (global-init constness check) and ir (pack_init) so that
 * constant expressions are accepted and emitted exactly like literals. */
int fold_const_int(const Expr *e, long long *out) {
    if (!e) return 0;
    if (e->kind == EX_INT_LIT) {
        *out = e->u.int_val;
        return 1;
    }
    if (e->kind == EX_CAST) {
        /* Fold through integer casts (e.g. `(int)`); pointer casts are not
         * integer constants, so require the operand to fold to an int. */
        return fold_const_int(e->u.cast.operand, out);
    }
    if (e->kind == EX_UNARY) {
        long long v;
        if (!fold_const_int(e->u.un.operand, &v)) return 0;
        switch (e->u.un.op) {
        case UOP_NEG: *out = -v; return 1;
        case UOP_POS: *out = v; return 1;
        case UOP_BITNOT: *out = ~v; return 1;
        case UOP_NOT: *out = !v ? 1 : 0; return 1;
        default: return 0;
        }
    }
    if (e->kind == EX_BINOP) {
        long long l, r;
        if (!fold_const_int(e->u.bin.l, &l)) return 0;
        if (!fold_const_int(e->u.bin.r, &r)) return 0;
        switch (e->u.bin.op) {
        case BOP_ADD: *out = l + r; return 1;
        case BOP_SUB: *out = l - r; return 1;
        case BOP_MUL: *out = l * r; return 1;
        case BOP_DIV: if (r == 0) { *out = 0; return 1; } *out = l / r; return 1;
        case BOP_MOD: if (r == 0) { *out = 0; return 1; } *out = l % r; return 1;
        case BOP_BITAND: *out = l & r; return 1;
        case BOP_BITOR:  *out = l | r; return 1;
        case BOP_BITXOR: *out = l ^ r; return 1;
        case BOP_SHL:    *out = l << r; return 1;
        case BOP_SHR:    *out = l >> r; return 1;
        case BOP_EQ:     *out = (l == r) ? 1 : 0; return 1;
        case BOP_NE:     *out = (l != r) ? 1 : 0; return 1;
        case BOP_LT:     *out = (l < r) ? 1 : 0; return 1;
        case BOP_LE:     *out = (l <= r) ? 1 : 0; return 1;
        case BOP_GT:     *out = (l > r) ? 1 : 0; return 1;
        case BOP_GE:     *out = (l >= r) ? 1 : 0; return 1;
        default: return 0;  /* BOP_AND/BOP_OR (logical) */
        }
    }
    if (e->kind == EX_SIZEOF_TYPE) {
        *out = type_size(e->u.sizeof_t.target);
        return 1;
    }
    if (e->kind == EX_ALIGNOF_TYPE) {
        *out = type_align(e->u.alignof_t.target);
        return 1;
    }
    return 0;
}
