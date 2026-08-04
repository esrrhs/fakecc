#include "fakecc/sema.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Set by sema_check; provides the module StructRegistry for member lookups. */
static const StructRegistry *g_sema_structs = NULL;

/* Return type of the function currently being type-checked.  Set per-function
 * before check_stmt_list so ST_RETURN can enforce the void/non-void rules. */
static Type g_sema_ret_type;

typedef struct {
    const char *name;
    int arity;
    Type ret_type;
    Type param_types[16];
    SourceLoc loc;
} FunSig;

typedef struct {
    FunSig *data;
    size_t len;
    size_t cap;
} FunTable;

static void ftab_init(FunTable *t) { t->data = NULL; t->len = 0; t->cap = 0; }
static void ftab_free(FunTable *t) { free(t->data); t->data = NULL; t->len = 0; t->cap = 0; }

static void ftab_push(FunTable *t, const FunctionDecl *fn) {
    if (t->len >= t->cap) {
        t->cap = t->cap ? t->cap * 2 : 8;
        t->data = realloc(t->data, t->cap * sizeof(FunSig));
        if (!t->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    FunSig *s = &t->data[t->len++];
    s->name = fn->name;
    s->arity = (int)fn->params.len;
    s->ret_type = fn->ret_type;
    s->loc = fn->loc;
    for (int i = 0; i < s->arity && i < 16; i++)
        s->param_types[i] = fn->params.data[i].type;
}

static const FunSig *ftab_find(const FunTable *t, const char *name) {
    for (size_t i = 0; i < t->len; i++)
        if (strcmp(t->data[i].name, name) == 0) return &t->data[i];
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Label set — tracks goto targets within one function for validation. */
/* ------------------------------------------------------------------ */

typedef struct {
    char **names;      /* xstrdup'd label names */
    size_t len;
    size_t cap;
} LabelSet;

static void labelset_init(LabelSet *ls) { ls->names = NULL; ls->len = 0; ls->cap = 0; }

static void labelset_free(LabelSet *ls) {
    for (size_t i = 0; i < ls->len; i++) free(ls->names[i]);
    free(ls->names);
    ls->names = NULL; ls->len = 0; ls->cap = 0;
}

static void labelset_add(LabelSet *ls, const char *name) {
    if (ls->len >= ls->cap) {
        ls->cap = ls->cap ? ls->cap * 2 : 8;
        ls->names = realloc(ls->names, ls->cap * sizeof(char *));
        if (!ls->names) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    ls->names[ls->len++] = xstrdup(name);
}

static int labelset_has(const LabelSet *ls, const char *name) {
    for (size_t i = 0; i < ls->len; i++)
        if (strcmp(ls->names[i], name) == 0) return 1;
    return 0;
}

/* Recursively collect every ST_LABEL name in a statement (forward goto must
 * be able to target labels that appear later in the function). */
static void collect_labels(LabelSet *ls, const Stmt *s) {
    if (!s) return;
    switch (s->kind) {
    case ST_LABEL:
        labelset_add(ls, s->u.label_s.name);
        collect_labels(ls, s->u.label_s.stmt);
        break;
    case ST_IF:
        collect_labels(ls, s->u.if_s.then_s);
        if (s->u.if_s.else_s) collect_labels(ls, s->u.if_s.else_s);
        break;
    case ST_WHILE:
        collect_labels(ls, s->u.while_s.body);
        break;
    case ST_DO_WHILE:
        collect_labels(ls, s->u.do_s.body);
        break;
    case ST_FOR:
        if (s->u.for_s.init) collect_labels(ls, s->u.for_s.init);
        collect_labels(ls, s->u.for_s.body);
        break;
    case ST_BLOCK:
        for (size_t i = 0; i < s->u.block.len; i++)
            collect_labels(ls, &s->u.block.data[i]);
        break;
    default:
        break;
    }
}

typedef struct {
    char *name;
    Type type;
    SourceLoc loc;
} Sym;

typedef struct {
    Sym *data;
    size_t len;
    size_t cap;
} SymTable;

static void symtable_init(SymTable *st) { st->data = NULL; st->len = 0; st->cap = 0; }

static void symtable_free(SymTable *st) {
    for (size_t i = 0; i < st->len; i++) {
        free(st->data[i].name);
        type_free(&st->data[i].type);
    }
    free(st->data);
    st->data = NULL; st->len = 0; st->cap = 0;
}

static void symtable_push(SymTable *st, const char *name, Type type, SourceLoc loc) {
    if (st->len >= st->cap) {
        st->cap = st->cap ? st->cap * 2 : 8;
        st->data = realloc(st->data, st->cap * sizeof(Sym));
        if (!st->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    st->data[st->len].name = name ? xstrdup(name) : NULL;
    st->data[st->len].type = type_clone(type);   /* own our copy */
    st->data[st->len].loc = loc;
    st->len++;
}

static size_t symtable_enter_scope(SymTable *st) { return st->len; }

static void symtable_leave_scope(SymTable *st, size_t mark) {
    while (st->len > mark) {
        st->len--;
        free(st->data[st->len].name);
        type_free(&st->data[st->len].type);
    }
}

static const Sym *symtable_find(const SymTable *st, const char *name) {
    for (size_t i = st->len; i > 0; i--) {
        const Sym *e = &st->data[i - 1];
        if (e->name && strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

static int symtable_has_since(const SymTable *st, const char *name, size_t mark) {
    for (size_t i = mark; i < st->len; i++)
        if (st->data[i].name && strcmp(st->data[i].name, name) == 0) return 1;
    return 0;
}

/* Usual arithmetic conversions (C §6.3.1.8), rank approximated by width. */
static Type integer_promote(Type t) {
    if (t.width < 4) return type_make_int(4, 0);
    return t;
}

static Type usual_arith_conv(Type a, Type b) {
    a = integer_promote(a);
    b = integer_promote(b);
    if (a.width == b.width && a.is_unsigned == b.is_unsigned) return a;
    if (a.is_unsigned == b.is_unsigned) return a.width > b.width ? a : b;
    Type u = a.is_unsigned ? a : b;
    Type s = a.is_unsigned ? b : a;
    if (u.width >= s.width) return u;
    return s;
}

/* Set an expr's type (frees previous). Convenience wrapper. */
static void set_type(Expr *e, Type t) { expr_set_type(e, t); }

/* Annotate e->type via type-checking; also returns a clone for use in callers. */
static Type check_expr(Expr *e, const SymTable *st, const FunTable *ft) {
    if (!e) return type_default_int();
    switch (e->kind) {
    case EX_INT_LIT:
        set_type(e, type_default_int());
        return type_clone(e->type);
    case EX_BINOP: {
        Type lt = check_expr(e->u.bin.l, st, ft);
        Type rt = check_expr(e->u.bin.r, st, ft);
        /* Array-to-pointer decay for operands (except & / sizeof handled elsewhere). */
        if (lt.kind == TY_ARRAY) {
            Type d = type_decay(lt); type_free(&lt); lt = d;
            /* Also update the child's e->type to reflect decay so IR-gen sees it. */
            set_type(e->u.bin.l, type_clone(lt));
        }
        if (rt.kind == TY_ARRAY) {
            Type d = type_decay(rt); type_free(&rt); rt = d;
            set_type(e->u.bin.r, type_clone(rt));
        }
        BinOp op = e->u.bin.op;
        Type res;
        if (op == BOP_AND || op == BOP_OR) {
            /* Logical && / ||: both operands must be scalar (int or pointer).
             * Result is always int 0 or 1. */
            if (lt.kind != TY_INT && lt.kind != TY_PTR)
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "left operand of '%s' must be scalar",
                       op == BOP_AND ? "&&" : "||");
            if (rt.kind != TY_INT && rt.kind != TY_PTR)
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "right operand of '%s' must be scalar",
                       op == BOP_AND ? "&&" : "||");
            res = type_make_int(4, 0);
        } else if (op >= BOP_EQ && op <= BOP_GE) {
            res = type_make_int(4, 0);
        } else if ((op == BOP_ADD || op == BOP_SUB) && (lt.kind == TY_PTR || rt.kind == TY_PTR)) {
            /* Pointer arithmetic: p+int, int+p, p-int → pointer; p-q → int. */
            if (op == BOP_SUB && lt.kind == TY_PTR && rt.kind == TY_PTR) {
                res = type_make_int(8, 0);   /* ptrdiff — use long */
            } else if (lt.kind == TY_PTR) {
                res = type_clone(lt);
            } else {
                res = type_clone(rt);
            }
        } else if (op == BOP_BITAND || op == BOP_BITOR || op == BOP_BITXOR) {
            /* Bitwise & | ^: both operands must be integer; result = UAC. */
            if (lt.kind != TY_INT)
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "left operand of '%s' must be integer",
                       op == BOP_BITAND ? "&" : op == BOP_BITOR ? "|" : "^");
            if (rt.kind != TY_INT)
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "right operand of '%s' must be integer",
                       op == BOP_BITAND ? "&" : op == BOP_BITOR ? "|" : "^");
            res = usual_arith_conv(lt, rt);
        } else if (op == BOP_SHL || op == BOP_SHR) {
            /* Shift << >>: both operands must be integer; result type is the
             * promoted type of the LEFT operand (C §6.5.7). */
            if (lt.kind != TY_INT)
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "left operand of '%s' must be integer",
                       op == BOP_SHL ? "<<" : ">>");
            if (rt.kind != TY_INT)
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "right operand of '%s' must be integer",
                       op == BOP_SHL ? "<<" : ">>");
            res = integer_promote(lt);
        } else {
            res = usual_arith_conv(lt, rt);
        }
        type_free(&lt); type_free(&rt);
        set_type(e, res);
        return type_clone(e->type);
    }
    case EX_UNARY: {
        Type ot = check_expr(e->u.un.operand, st, ft);
        if (ot.kind == TY_ARRAY) {
            Type d = type_decay(ot); type_free(&ot); ot = d;
            set_type(e->u.un.operand, type_clone(ot));
        }
        /* Bitwise NOT requires an integer operand; +/- on pointer is handled
         * in BOP. Here -, +, ~ require scalar integer; reject pointer. */
        if ((e->u.un.op == UOP_BITNOT) && ot.kind != TY_INT)
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "bitwise NOT requires an integer operand");
        Type res;
        if (e->u.un.op == UOP_NOT) {
            /* Logical NOT: operand must be scalar (int or pointer); result is
             * always int 0 or 1. */
            if (ot.kind != TY_INT && ot.kind != TY_PTR)
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "logical NOT requires a scalar operand");
            res = type_make_int(4, 0);
        } else {
            res = integer_promote(ot);
        }
        type_free(&ot);
        set_type(e, res);
        return type_clone(e->type);
    }
    case EX_VAR: {
        const Sym *sym = symtable_find(st, e->u.var.name);
        if (sym) {
            set_type(e, type_clone(sym->type));
            return type_clone(e->type);
        }
        /* Not a variable — is it a function name?  A function lvalue decays
         * to a pointer to its type (so `add` and &add both yield `ptr(func)`). */
        const FunSig *sig = ftab_find(ft, e->u.var.name);
        if (sig) {
            Type *ptys = NULL;
            if (sig->arity > 0) {
                ptys = malloc(sig->arity * sizeof(Type));
                if (!ptys) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
                for (int i = 0; i < sig->arity; i++)
                    ptys[i] = sig->param_types[i];
            }
            Type fn = type_make_func(sig->ret_type, ptys, sig->arity);
            if (ptys) free(ptys);
            Type fp = type_make_ptr(fn);
            type_free(&fn);
            set_type(e, fp);
            return type_clone(e->type);
        }
        die_at(e->loc.file, e->loc.line, e->loc.col,
               "use of undeclared variable '%s'", e->u.var.name);
    }
    case EX_ASSIGN: {
        if (e->u.assign.lvalue->kind != EX_VAR &&
            e->u.assign.lvalue->kind != EX_DEREF &&
            e->u.assign.lvalue->kind != EX_INDEX &&
            e->u.assign.lvalue->kind != EX_MEMBER) {
            die_at(e->u.assign.lvalue->loc.file,
                   e->u.assign.lvalue->loc.line,
                   e->u.assign.lvalue->loc.col,
                   "expression is not assignable");
        }
        Type lt = check_expr(e->u.assign.lvalue, st, ft);
        if (lt.is_const)
            die_at(e->u.assign.lvalue->loc.file,
                   e->u.assign.lvalue->loc.line,
                   e->u.assign.lvalue->loc.col,
                   "assignment of read-only variable");
        Type rt = check_expr(e->u.assign.rvalue, st, ft);
        (void)rt;
        type_free(&rt);
        set_type(e, lt);
        return type_clone(e->type);
    }
    case EX_COMPOUND_ASSIGN: {
        /* lvalue op= rvalue. The lvalue must be assignable. Type rules follow
         * the underlying binary op: +=/-= allow pointer += int; the rest
         * require integer operands. Result type is the lvalue's type. */
        Expr *lv = e->u.comp.lvalue;
        if (lv->kind != EX_VAR && lv->kind != EX_DEREF &&
            lv->kind != EX_INDEX && lv->kind != EX_MEMBER)
            die_at(lv->loc.file, lv->loc.line, lv->loc.col,
                   "left operand of '%s' must be an lvalue",
                   "compound assign");
        Type lt = check_expr(lv, st, ft);
        if (lt.is_const)
            die_at(lv->loc.file, lv->loc.line, lv->loc.col,
                   "compound assignment of read-only variable");
        Type rt = check_expr(e->u.comp.rvalue, st, ft);
        BinOp op = e->u.comp.op;
        if (op == BOP_ADD || op == BOP_SUB) {
            /* Pointer arithmetic: p += n, p -= n (n must be int). */
            if (lt.kind == TY_PTR && rt.kind != TY_INT)
                die_at(lv->loc.file, lv->loc.line, lv->loc.col,
                       "pointer %s requires an integer right operand",
                       op == BOP_ADD ? "+=" : "-=");
        } else {
            if (lt.kind != TY_INT)
                die_at(lv->loc.file, lv->loc.line, lv->loc.col,
                       "left operand of '%s' must be integer",
                       "compound assign");
            if (rt.kind != TY_INT)
                die_at(lv->loc.file, lv->loc.line, lv->loc.col,
                       "right operand of '%s' must be integer",
                       "compound assign");
        }
        type_free(&rt);
        set_type(e, lt);
        return type_clone(e->type);
    }
    case EX_CALL: {
        /* Recognize the __syscall intrinsic FIRST — before type-checking the
         * callee, because `__syscall` is not a real variable/function and would
         * otherwise fail the EX_VAR lookup.  It takes 1..7 int args and returns
         * long. */
        if (e->u.call.callee->kind == EX_VAR
            && strcmp(e->u.call.callee->u.var.name, "__syscall") == 0) {
            if (e->u.call.args.len < 1 || e->u.call.args.len > 7) {
                die_at(e->loc.file, e->loc.line, e->loc.col,
                       "__syscall takes 1 to 7 arguments (syscall num + up to 6 args)");
            }
            for (size_t i = 0; i < e->u.call.args.len; i++) {
                Type at = check_expr(e->u.call.args.data[i], st, ft);
                type_free(&at);
            }
            set_type(e, type_make_int(8, 0));
            return type_clone(e->type);
        }
        /* Type-check the callee expression. */
        Type callee_ty = check_expr(e->u.call.callee, st, ft);
        /* Direct call: callee is `EX_VAR` naming a known function. */
        if (e->u.call.callee->kind == EX_VAR) {
            const FunSig *sig = ftab_find(ft, e->u.call.callee->u.var.name);
            if (sig) {
                type_free(&callee_ty);
                if ((int)e->u.call.args.len != sig->arity) {
                    die_at(e->loc.file, e->loc.line, e->loc.col,
                           "function '%s' takes %d argument%s but %zu given",
                           e->u.call.callee->u.var.name, sig->arity,
                           sig->arity == 1 ? "" : "s", e->u.call.args.len);
                }
                for (size_t i = 0; i < e->u.call.args.len; i++) {
                    Type at = check_expr(e->u.call.args.data[i], st, ft);
                    type_free(&at);
                }
                set_type(e, type_clone(sig->ret_type));
                return type_clone(e->type);
            }
        }
        /* Indirect call: callee type must be pointer-to-function or bare
         * function (a function lvalue such as `*fp` decays to a pointer). */
        Type fn_ty = callee_ty;
        if (fn_ty.kind == TY_PTR && fn_ty.pointee && fn_ty.pointee->kind == TY_FUNC) {
            fn_ty = type_clone(*fn_ty.pointee);
            type_free(&callee_ty);
        } else if (fn_ty.kind == TY_FUNC) {
            /* Bare function lvalue — keep as-is. */
            fn_ty = type_clone(callee_ty);
            type_free(&callee_ty);
        } else {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "call to non-function (callee type must be a function or function pointer)");
        }
        if ((int)e->u.call.args.len != fn_ty.func_nparams) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "function pointer expects %d argument%s but %zu given",
                   fn_ty.func_nparams,
                   fn_ty.func_nparams == 1 ? "" : "s", e->u.call.args.len);
        }
        for (size_t i = 0; i < e->u.call.args.len; i++) {
            Type at = check_expr(e->u.call.args.data[i], st, ft);
            type_free(&at);
        }
        set_type(e, type_clone(*fn_ty.func_ret));
        type_free(&fn_ty);
        return type_clone(e->type);
    }
    case EX_STR: {
        /* Type: char[len+1] — but immediately decayed to `char*` in the
         * value context. Since decay happens on read at operand sites, we
         * store the pointer type here directly. */
        Type ct = type_make_int(1, 0);   /* char */
        set_type(e, type_make_ptr(ct));
        type_free(&ct);
        return type_clone(e->type);
    }
    case EX_ADDR: {
        Type ot = check_expr(e->u.addr.operand, st, ft);
        /* operand must be lvalue */
        ExprKind ok = e->u.addr.operand->kind;
        if (ok != EX_VAR && ok != EX_DEREF && ok != EX_INDEX && ok != EX_MEMBER) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "cannot take address of rvalue");
        }
        /* `&function` is a no-op in C -- the result is the same function
         * pointer that the function lvalue already decayed to.  If the operand
         * is already a function pointer (from decay), just return it. */
        if (ot.kind == TY_PTR && ot.pointee && ot.pointee->kind == TY_FUNC) {
            set_type(e, ot);  /* takes ownership of ot */
            return type_clone(e->type);
        }
        Type res = type_make_ptr(ot);
        type_free(&ot);
        set_type(e, res);
        return type_clone(e->type);
    }
    case EX_DEREF: {
        Type ot = check_expr(e->u.deref.operand, st, ft);
        if (ot.kind == TY_ARRAY) {
            Type d = type_decay(ot); type_free(&ot); ot = d;
            set_type(e->u.deref.operand, type_clone(ot));
        }
        if (ot.kind != TY_PTR) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "cannot dereference non-pointer");
        }
        Type res = type_clone(*ot.pointee);
        type_free(&ot);
        set_type(e, res);
        return type_clone(e->type);
    }
    case EX_INDEX: {
        /* a[i] — a must be ptr or array; result = pointee/elem. */
        Type at = check_expr(e->u.idx.array, st, ft);
        Type it = check_expr(e->u.idx.index, st, ft);
        (void)it;
        type_free(&it);
        Type base = at;
        if (base.kind == TY_ARRAY) {
            /* Decay to pointer *locally* for typing.  Do NOT overwrite the
             * subexpression's type — IR-gen needs to see the original array
             * type to decide whether to return an address (for nested a[i][j]
             * where inner a[i] is another array) or to load a value. */
            Type d = type_decay(base); type_free(&base); base = d;
        }
        if (base.kind != TY_PTR) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "subscript on non-pointer/non-array");
        }
        Type res = type_clone(*base.pointee);
        type_free(&base);
        set_type(e, res);
        return type_clone(e->type);
    }
    case EX_MEMBER: {
        Type ot = check_expr(e->u.member.obj, st, ft);
        if (ot.kind != TY_STRUCT) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "member access '.%s' on non-struct", e->u.member.name);
        }
        const StructDef *sd = struct_registry_find_c(g_sema_structs, ot.tag);
        if (!sd) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "unknown struct 'struct %s'", ot.tag);
        }
        const StructMember *m = NULL;
        for (int i = 0; i < sd->num_members; i++)
            if (strcmp(sd->members[i].name, e->u.member.name) == 0)
                { m = &sd->members[i]; break; }
        if (!m) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "struct '%s' has no member '%s'", ot.tag, e->u.member.name);
        }
        type_free(&ot);
        set_type(e, type_clone(m->type));
        return type_clone(e->type);
    }
    case EX_INC_DEC: {
        /* ++lvalue / --lvalue (prefix or postfix). */
        Expr *op = e->u.incdec.operand;
        if (op->kind != EX_VAR && op->kind != EX_DEREF &&
            op->kind != EX_INDEX && op->kind != EX_MEMBER)
            die_at(op->loc.file, op->loc.line, op->loc.col,
                   "operand of '%s' must be an lvalue",
                   e->u.incdec.is_inc ? "++" : "--");
        Type ot = check_expr(op, st, ft);
        if (ot.is_const)
            die_at(op->loc.file, op->loc.line, op->loc.col,
                   "cannot increment/decrement a read-only variable");
        if (ot.kind != TY_INT && ot.kind != TY_PTR)
            die_at(op->loc.file, op->loc.line, op->loc.col,
                   "operand of '%s' must be int or pointer",
                   e->u.incdec.is_inc ? "++" : "--");
        type_free(&ot);
        /* Result type is the operand type (before increment). For both prefix
         * and postfix, e->type is the same as the operand's declared type. */
        Type res;
        if (op->type.kind == TY_PTR)
            res = type_clone(op->type);
        else
            res = type_make_int(op->type.width ? op->type.width : 4,
                                op->type.is_unsigned);
        set_type(e, res);
        return type_clone(e->type);
    }
    case EX_COMMA: {
        /* a, b: evaluate a (discard result), result is b. */
        Type lt = check_expr(e->u.comma.lhs, st, ft);
        type_free(&lt);
        Type rt = check_expr(e->u.comma.rhs, st, ft);
        set_type(e, rt);
        return type_clone(e->type);
    }
    case EX_TERNARY: {
        /* cond ? then : else
         * Condition must be scalar (int or pointer).  Result type follows
         * C §6.5.15: arithmetic operands → UAC; both pointers → that pointer
         * type; one pointer + null pointer constant (integer 0) → pointer type. */
        Type ct = check_expr(e->u.tern.cond, st, ft);
        if (ct.kind != TY_INT && ct.kind != TY_PTR)
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "ternary condition must be scalar");
        type_free(&ct);
        Type tt = check_expr(e->u.tern.then, st, ft);
        Type et = check_expr(e->u.tern.else_, st, ft);
        int t_is_null_const = (tt.kind == TY_INT && tt.width == 4
                               && e->u.tern.then->kind == EX_INT_LIT
                               && e->u.tern.then->u.int_val == 0);
        int e_is_null_const = (et.kind == TY_INT && et.width == 4
                               && e->u.tern.else_->kind == EX_INT_LIT
                               && e->u.tern.else_->u.int_val == 0);
        Type res;
        if (tt.kind == TY_INT && et.kind == TY_INT) {
            res = usual_arith_conv(tt, et);
        } else if (tt.kind == TY_PTR && et.kind == TY_PTR) {
            res = type_clone(tt);
        } else if (tt.kind == TY_PTR && e_is_null_const) {
            res = type_clone(tt);
        } else if (et.kind == TY_PTR && t_is_null_const) {
            res = type_clone(et);
        } else {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "ternary branches must both be int, both be pointer, "
                   "or pointer with null constant");
        }
        type_free(&tt); type_free(&et);
        set_type(e, res);
        return type_clone(e->type);
    }
    case EX_CAST: {
        Type ot = check_expr(e->u.cast.operand, st, ft);
        type_free(&ot);
        /* Result type is the cast target already stored on the expr;
         * we also mirror it on e->type. */
        set_type(e, type_clone(e->u.cast.target));
        return type_clone(e->type);
    }
    case EX_SIZEOF_TYPE:
    case EX_SIZEOF_EXPR: {
        if (e->kind == EX_SIZEOF_EXPR) {
            Type ot = check_expr(e->u.sizeof_e.operand, st, ft);
            type_free(&ot);
        }
        set_type(e, type_make_int(8, 1));   /* size_t == unsigned long */
        return type_clone(e->type);
    }
    case EX_INIT_LIST: {
        /* An initializer list is valid only as a declarator initializer.  We
         * type-check every element; the count / layout validation against the
         * target type happens in check_stmt(ST_DECL).  The list's own type is
         * the target type, which the caller knows — here we just return a
         * benign int so a stray use in an expression context degrades gracefully
         * (it is rejected upstream anyway). */
        for (int i = 0; i < e->u.init_list.num_elements; i++) {
            Type et = check_expr(e->u.init_list.elements[i], st, ft);
            type_free(&et);
        }
        set_type(e, type_default_int());
        return type_clone(e->type);
    }
    }
    return type_default_int();
}

static void check_stmt(Stmt *s, SymTable *st, const FunTable *ft,
                       size_t scope_mark, int *has_return);

static int g_sema_loop_depth = 0;
static LabelSet *g_sema_labels = NULL;

static void check_stmt_list(StmtArray *body, SymTable *st,
                            const FunTable *ft, int *has_return) {
    size_t mark = symtable_enter_scope(st);
    for (size_t i = 0; i < body->len; i++)
        check_stmt(&body->data[i], st, ft, mark, has_return);
    symtable_leave_scope(st, mark);
}

/* Is `e` a compile-time constant suitable for initializing a global?  Integer
 * literals and nested initializer lists of constants qualify. */
static int is_const_init(const Expr *e) {
    if (!e) return 1;
    if (e->kind == EX_INT_LIT) return 1;
    if (e->kind == EX_INIT_LIST) {
        for (int i = 0; i < e->u.init_list.num_elements; i++)
            if (!is_const_init(e->u.init_list.elements[i])) return 0;
        return 1;
    }
    return 0;
}

/* Validate an initializer list's shape against the target type: element count
 * must not exceed the array length / struct member count, and nested lists
 * recurse.  A scalar target tolerates a single-element brace list (e.g.
 * `int x = {5}`).  Dies on mismatch. */
static void check_init_list_shape(Type target, const Expr *list, SourceLoc loc) {
    int n = list->u.init_list.num_elements;
    switch (target.kind) {
    case TY_ARRAY:
        if (target.length > 0 && n > target.length)
            die_at(loc.file, loc.line, loc.col,
                   "too many initializers for array (expected %d, got %d)",
                   target.length, n);
        for (int i = 0; i < n; i++) {
            const Expr *elem = list->u.init_list.elements[i];
            if (elem->kind == EX_INIT_LIST && target.elem_type)
                check_init_list_shape(*target.elem_type, elem, elem->loc);
        }
        break;
    case TY_STRUCT: {
        const StructDef *sd = struct_registry_find_c(g_sema_structs, target.tag);
        if (!sd)
            die_at(loc.file, loc.line, loc.col,
                   "unknown struct 'struct %s'", target.tag);
        if (n > sd->num_members)
            die_at(loc.file, loc.line, loc.col,
                   "too many initializers for struct '%s' (expected %d, got %d)",
                   target.tag, sd->num_members, n);
        for (int i = 0; i < n; i++) {
            const Expr *elem = list->u.init_list.elements[i];
            if (elem->kind == EX_INIT_LIST && i < sd->num_members)
                check_init_list_shape(sd->members[i].type, elem, elem->loc);
        }
        break;
    }
    default:
        /* Scalar target: brace list with a single element is allowed. */
        if (n > 1)
            die_at(loc.file, loc.line, loc.col,
                   "scalar initializer requires at most one element");
        break;
    }
}

static void check_stmt(Stmt *s, SymTable *st, const FunTable *ft,
                       size_t scope_mark, int *has_return) {
    Type discard;
    switch (s->kind) {
    case ST_DECL: {
        if (symtable_has_since(st, s->u.decl.name, scope_mark)) {
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "redeclaration of '%s'", s->u.decl.name);
        }
        /* A variable may not have type `void` (but `void*` is fine — that is a
         * pointer, TY_PTR). */
        if (s->u.decl.type.kind == TY_VOID)
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "cannot declare variable of type void");
        /* `extern` is a file-scope-only declaration; a local `extern` has no
         * storage to bind to in this single-TU model.  check_stmt is only
         * reached for function-locals, so any `extern` here is an error. */
        if (s->u.decl.storage_class == 2)
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "'extern' is not allowed at block scope");
        /* Infer array length from an empty `[]` declarator when followed by an
         * initializer: `int a[] = {1,2,3}` → length 3, or
         * `char s[] = "hi"` → length strlen+1. */
        if (s->u.decl.type.kind == TY_ARRAY && s->u.decl.type.length == 0
            && s->u.decl.init) {
            if (s->u.decl.init->kind == EX_INIT_LIST) {
                s->u.decl.type.length = s->u.decl.init->u.init_list.num_elements;
            } else if (s->u.decl.init->kind == EX_STR
                       && s->u.decl.type.elem_type
                       && s->u.decl.type.elem_type->width == 1) {
                s->u.decl.type.length = s->u.decl.init->u.str.len + 1;
            }
        }
        /* A string literal initializing a `char[]` is lowered as a per-byte
         * copy, not a pointer store — convert it to an init list of char
         * literals so it flows through the array-init path. */
        if (s->u.decl.init && s->u.decl.init->kind == EX_STR
            && s->u.decl.type.kind == TY_ARRAY && s->u.decl.type.elem_type
            && s->u.decl.type.elem_type->width == 1) {
            Expr *str = s->u.decl.init;
            int n = str->u.str.len + 1;   /* include trailing NUL */
            Expr **elems = malloc(n * sizeof(Expr *));
            for (int i = 0; i < n - 1; i++)
                elems[i] = expr_new_int((unsigned char)str->u.str.bytes[i],
                                        str->loc);
            elems[n - 1] = expr_new_int(0, str->loc);   /* NUL terminator */
            Expr *list = expr_new_init_list(elems, n, str->loc);
            expr_free(str);
            s->u.decl.init = list;
        }
        symtable_push(st, s->u.decl.name, s->u.decl.type, s->loc);
        if (s->u.decl.init) {
            if (s->u.decl.init->kind == EX_INIT_LIST)
                check_init_list_shape(s->u.decl.type, s->u.decl.init, s->loc);
            discard = check_expr(s->u.decl.init, st, ft); type_free(&discard);
        }
        break;
    }
    case ST_EXPR:
        discard = check_expr(s->u.expr, st, ft); type_free(&discard);
        break;
    case ST_RETURN:
        /* Bare `return;` (value==NULL) is allowed only in a void function;
         * `return expr;` is forbidden in a void function.  Otherwise type-check
         * the returned expression as usual. */
        if (s->u.value == NULL) {
            if (g_sema_ret_type.kind != TY_VOID)
                die_at(s->loc.file, s->loc.line, s->loc.col,
                       "non-void function must return a value");
        } else {
            if (g_sema_ret_type.kind == TY_VOID)
                die_at(s->loc.file, s->loc.line, s->loc.col,
                       "void function cannot return a value");
            discard = check_expr(s->u.value, st, ft); type_free(&discard);
        }
        *has_return = 1;
        break;
    case ST_IF:
        discard = check_expr(s->u.if_s.cond, st, ft); type_free(&discard);
        check_stmt(s->u.if_s.then_s, st, ft, scope_mark, has_return);
        if (s->u.if_s.else_s) check_stmt(s->u.if_s.else_s, st, ft, scope_mark, has_return);
        break;
    case ST_WHILE:
        discard = check_expr(s->u.while_s.cond, st, ft); type_free(&discard);
        g_sema_loop_depth++;
        check_stmt(s->u.while_s.body, st, ft, scope_mark, has_return);
        g_sema_loop_depth--;
        break;
    case ST_DO_WHILE:
        /* Condition must be scalar (int or pointer). */
        discard = check_expr(s->u.do_s.cond, st, ft);
        if (discard.kind != TY_INT && discard.kind != TY_PTR)
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "do-while condition must be scalar");
        type_free(&discard);
        g_sema_loop_depth++;
        check_stmt(s->u.do_s.body, st, ft, scope_mark, has_return);
        g_sema_loop_depth--;
        break;
    case ST_GOTO:
        if (!labelset_has(g_sema_labels, s->u.goto_s.target)) {
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "use of undeclared label '%s'", s->u.goto_s.target);
        }
        break;
    case ST_LABEL:
        /* Validate the wrapped statement; the label name itself was
         * registered during the collect_labels pre-pass. */
        check_stmt(s->u.label_s.stmt, st, ft, scope_mark, has_return);
        break;
    case ST_SWITCH: {
        /* Condition must be integer. */
        Type ct = check_expr(s->u.switch_s.cond, st, ft);
        if (ct.kind != TY_INT)
            die_at(s->u.switch_s.cond->loc.file,
                   s->u.switch_s.cond->loc.line,
                   s->u.switch_s.cond->loc.col,
                   "switch condition must be integer");
        type_free(&ct);
        /* Switch introduces a breakable scope. */
        g_sema_loop_depth++;
        for (int i = 0; i < s->u.switch_s.num_cases; i++) {
            SwitchCase *arm = &s->u.switch_s.cases[i];
            for (size_t j = 0; j < arm->stmts.len; j++)
                check_stmt(&arm->stmts.data[j], st, ft, scope_mark, has_return);
        }
        g_sema_loop_depth--;
        break;
    }
    case ST_FOR: {
        /* for-loop introduces its own scope for the init decl (if any). */
        size_t mark = symtable_enter_scope(st);
        if (s->u.for_s.init) {
            check_stmt(s->u.for_s.init, st, ft, mark, has_return);
        }
        if (s->u.for_s.cond) {
            discard = check_expr(s->u.for_s.cond, st, ft); type_free(&discard);
        }
        if (s->u.for_s.step) {
            discard = check_expr(s->u.for_s.step, st, ft); type_free(&discard);
        }
        g_sema_loop_depth++;
        check_stmt(s->u.for_s.body, st, ft, mark, has_return);
        g_sema_loop_depth--;
        symtable_leave_scope(st, mark);
        break;
    }
    case ST_BREAK:
    case ST_CONTINUE:
        if (g_sema_loop_depth == 0) {
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "'%s' outside of loop", s->kind == ST_BREAK ? "break" : "continue");
        }
        break;
    case ST_BLOCK:
        check_stmt_list(&s->u.block, st, ft, has_return);
        break;
    }
}

void sema_check(const TranslationUnit *tu_const) {
    TranslationUnit *tu = (TranslationUnit *)tu_const;
    g_sema_structs = &tu->structs;

    if (tu->package.name == NULL || strcmp(tu->package.name, "main") != 0) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "package must be 'main'");
    }

    FunTable ft;
    ftab_init(&ft);
    int has_main = 0;
    for (size_t i = 0; i < tu->functions.len; i++) {
        FunctionDecl *fn = &tu->functions.data[i];
        if (ftab_find(&ft, fn->name)) {
            die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                   "redefinition of function '%s'", fn->name);
        }
        ftab_push(&ft, fn);
        if (strcmp(fn->name, "main") == 0) has_main = 1;
    }
    if (!has_main) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "no 'main' function defined");
    }

    /* Type-check global variable declarations.  Store their names in a
     * long-lived symbol table so functions can see them.  Enforce no
     * duplicates against each other or against function names. */
    SymTable globals;
    symtable_init(&globals);
    for (size_t i = 0; i < tu->globals.len; i++) {
        Stmt *s = &tu->globals.data[i];
        if (s->kind != ST_DECL) continue;
        if (symtable_has_since(&globals, s->u.decl.name, 0)) {
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "redefinition of global '%s'", s->u.decl.name);
        }
        if (ftab_find(&ft, s->u.decl.name)) {
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "global '%s' conflicts with a function of the same name",
                   s->u.decl.name);
        }
        /* Infer array length from an empty `[]` declarator when followed by an
         * initializer: `int a[] = {1,2,3}` → length 3, or
         * `char s[] = "hi"` → length strlen+1. */
        if (s->u.decl.type.kind == TY_ARRAY && s->u.decl.type.length == 0
            && s->u.decl.init) {
            if (s->u.decl.init->kind == EX_INIT_LIST) {
                s->u.decl.type.length = s->u.decl.init->u.init_list.num_elements;
            } else if (s->u.decl.init->kind == EX_STR
                       && s->u.decl.type.elem_type
                       && s->u.decl.type.elem_type->width == 1) {
                s->u.decl.type.length = s->u.decl.init->u.str.len + 1;
            }
        }
        symtable_push(&globals, s->u.decl.name, s->u.decl.type, s->loc);
        if (s->u.decl.init) {
            /* Validate the initializer list shape against the declared type. */
            if (s->u.decl.init->kind == EX_INIT_LIST)
                check_init_list_shape(s->u.decl.type, s->u.decl.init, s->loc);
            /* A global's initializer must be a compile-time constant. */
            if (!is_const_init(s->u.decl.init))
                die_at(s->loc.file, s->loc.line, s->loc.col,
                       "global initializer must be a constant");
            Type dt = check_expr(s->u.decl.init, &globals, &ft);
            type_free(&dt);
        }
    }

    for (size_t i = 0; i < tu->functions.len; i++) {
        FunctionDecl *fn = &tu->functions.data[i];

        if (strcmp(fn->name, "main") == 0 && fn->params.len != 0) {
            die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                   "'main' must take no parameters");
        }

        SymTable st;
        symtable_init(&st);
        /* Import globals (as a fixed lower scope). */
        for (size_t g = 0; g < globals.len; g++) {
            symtable_push(&st, globals.data[g].name, globals.data[g].type,
                          globals.data[g].loc);
        }

        size_t mark = symtable_enter_scope(&st);
        for (size_t j = 0; j < fn->params.len; j++) {
            if (symtable_has_since(&st, fn->params.data[j].name, mark)) {
                die_at(fn->params.data[j].loc.file,
                       fn->params.data[j].loc.line,
                       fn->params.data[j].loc.col,
                       "duplicate parameter name '%s'",
                       fn->params.data[j].name);
            }
            /* Normalize unsized array parameters (`int a[]`) to pointers,
             * matching standard C parameter adjustment. */
            Type pty = fn->params.data[j].type;
            if (pty.kind == TY_ARRAY && pty.length == 0) {
                pty = type_make_ptr(*pty.elem_type);
            }
            symtable_push(&st, fn->params.data[j].name,
                          pty, fn->params.data[j].loc);
            if (pty.kind == TY_PTR) type_free(&pty);
        }

        /* Pre-pass: collect every label in the function so forward gotos
         * resolve. */
        LabelSet ls;
        labelset_init(&ls);
        g_sema_labels = &ls;
        for (size_t j = 0; j < fn->body.len; j++)
            collect_labels(&ls, &fn->body.data[j]);

        g_sema_ret_type = fn->ret_type;
        int has_return = 0;
        check_stmt_list(&fn->body, &st, &ft, &has_return);
        symtable_leave_scope(&st, mark);
        symtable_free(&st);

        g_sema_labels = NULL;
        labelset_free(&ls);

        /* A void function need not return a value; a non-void function must
         * have a return statement. */
        if (!has_return && fn->ret_type.kind != TY_VOID) {
            die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                   "function '%s' must have a return statement", fn->name);
        }
    }

    ftab_free(&ft);
    symtable_free(&globals);
}
