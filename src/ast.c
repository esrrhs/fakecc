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
        r.pointee = malloc(sizeof(Type));
        if (!r.pointee) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        *r.pointee = type_clone(*t.pointee);
    } else {
        r.pointee = NULL;
    }
    if (t.kind == TY_ARRAY && t.elem_type) {
        r.elem_type = malloc(sizeof(Type));
        if (!r.elem_type) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        *r.elem_type = type_clone(*t.elem_type);
    } else {
        r.elem_type = NULL;
    }
    r.tag = t.tag ? xstrdup(t.tag) : NULL;
    return r;
}

void type_free(Type *t) {
    if (!t) return;
    if (t->pointee) { type_free(t->pointee); free(t->pointee); t->pointee = NULL; }
    if (t->elem_type) { type_free(t->elem_type); free(t->elem_type); t->elem_type = NULL; }
    if (t->tag) { free(t->tag); t->tag = NULL; }
}

int type_size(Type t) {
    switch (t.kind) {
    case TY_INT:   return t.width;
    case TY_PTR:   return 8;
    case TY_ARRAY: return type_size(*t.elem_type) * t.length;
    case TY_STRUCT: return t.width;  /* precomputed at struct-def time */
    }
    return 0;
}

Type type_make_ptr(Type pointee) {
    Type t; t.kind = TY_PTR; t.width = 8; t.is_unsigned = 1;
    t.is_const = 0;
    t.elem_type = NULL; t.length = 0; t.tag = NULL;
    t.pointee = malloc(sizeof(Type));
    if (!t.pointee) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *t.pointee = type_clone(pointee);
    return t;
}

Type type_make_array(Type elem, int length) {
    Type t; t.kind = TY_ARRAY; t.width = elem.width;
    t.is_unsigned = elem.is_unsigned; t.is_const = 0; t.length = length;
    t.pointee = NULL; t.tag = NULL;
    t.elem_type = malloc(sizeof(Type));
    if (!t.elem_type) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *t.elem_type = type_clone(elem);
    return t;
}

Type type_make_struct(const char *tag, int size) {
    Type t; t.kind = TY_STRUCT; t.width = size; t.is_unsigned = 0;
    t.is_const = 0;
    t.pointee = NULL; t.elem_type = NULL; t.length = 0;
    t.tag = xstrdup(tag);
    return t;
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
    sd->size = 0; sd->loc = loc;
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
static int type_align(Type t) {
    switch (t.kind) {
    case TY_INT:   return t.width;
    case TY_PTR:   return 8;
    case TY_ARRAY: return type_align(*t.elem_type);
    case TY_STRUCT: return 8;   /* conservative — structs align to 8 */
    }
    return 1;
}

void struct_def_push_member(StructDef *sd, const char *name, Type ty) {
    if (sd->num_members >= sd->cap_members) {
        int nc = sd->cap_members ? sd->cap_members * 2 : 4;
        sd->members = realloc(sd->members, nc * sizeof(StructMember));
        if (!sd->members) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        sd->cap_members = nc;
    }
    int a = type_align(ty);
    int sz = type_size(ty);
    int off;
    if (sd->is_union) {
        /* Union members all start at offset 0; total size is the max. */
        off = 0;
    } else {
        off = align_up(sd->size, a);
    }
    sd->members[sd->num_members].name = xstrdup(name);
    sd->members[sd->num_members].type = type_clone(ty);
    sd->members[sd->num_members].offset = off;
    sd->num_members++;
    if (sd->is_union) {
        /* Size grows to the largest member (aligned at the end). */
        if (sz > sd->size) sd->size = sz;
        sd->size = align_up(sd->size, 8);
    } else {
        sd->size = off + sz;
        /* pad struct to 8-byte boundary at end */
        sd->size = align_up(sd->size, 8);
    }
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
/* Expr constructors & destructor                                       */
/* ------------------------------------------------------------------ */

Expr *expr_new_int(int v, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_INT_LIT;
    e->loc = loc;
    e->type = type_default_int();
    e->u.int_val = v;
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
    e->u.var.name = xstrdup(name);
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
    e->u.assign.lvalue = lvalue;
    e->u.assign.rvalue = rvalue;
    return e;
}

Expr *expr_new_call(const char *callee, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_CALL;
    e->loc = loc;
    e->type = type_default_int();
    e->u.call.callee = xstrdup(callee);
    e->u.call.args.data = NULL;
    e->u.call.args.len = 0;
    e->u.call.args.cap = 0;
    return e;
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
        break;
    case EX_ASSIGN:
        expr_free(e->u.assign.lvalue);
        expr_free(e->u.assign.rvalue);
        break;
    case EX_CALL:
        free(e->u.call.callee);
        for (size_t i = 0; i < e->u.call.args.len; i++)
            expr_free(e->u.call.args.data[i]);
        free(e->u.call.args.data);
        break;
    case EX_STR:
        free(e->u.str.bytes);
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

void tu_init(TranslationUnit *tu) {
    tu->package.name = NULL;
    tu->package.loc.file = NULL;
    tu->package.loc.line = 0;
    tu->package.loc.col = 0;
    stmt_array_init(&tu->globals);
    tu->functions.data = NULL;
    tu->functions.len = 0;
    tu->functions.cap = 0;
    struct_registry_init(&tu->structs);
    enum_registry_init(&tu->enums);
}

void tu_free(TranslationUnit *tu) {
    free(tu->package.name);
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
