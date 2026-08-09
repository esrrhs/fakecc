#ifndef FAKECC_AST_H
#define FAKECC_AST_H

#include "fakecc/token.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Type — 7a: integer types; 7b/c: pointers + arrays                    */
/* ------------------------------------------------------------------ */

typedef enum {
    TY_VOID,    /* void — only valid as a function return type */
    TY_INT,     /* char/short/int/long × signed/unsigned */
    TY_FLOAT,   /* float (width 4) / double (width 8) */
    TY_PTR,     /* T*  — pointee owned via heap allocation */
    TY_ARRAY,   /* T[N] — elem_type owned via heap; length is a positive int */
    TY_STRUCT,  /* struct Name — layout looked up in module StructRegistry */
    TY_FUNC,    /* function type: ret(params...). Carried as pointee of a TY_PTR. */
} TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    int width;         /* TY_INT: 1/2/4/8. TY_PTR: always 8. TY_ARRAY: elem width. TY_STRUCT: total size. TY_FUNC: 0. */
    int is_unsigned;   /* TY_INT only */
    unsigned is_const : 1; /* const-qualified (assignment forbidden) */
    unsigned is_volatile : 1; /* volatile-qualified (no-op without an optimizer) */
    unsigned is_restrict : 1; /* restrict-qualified (no-op without an optimizer) */
    unsigned is_bool : 1;  /* _Bool (width-1 unsigned that normalizes to 0/1) */
    Type *pointee;     /* TY_PTR only: malloc'd */
    Type *elem_type;   /* TY_ARRAY only: malloc'd */
    int length;        /* TY_ARRAY only */
    char *tag;         /* TY_STRUCT only: xstrdup'd tag name */
    Type *func_ret;    /* TY_FUNC only: malloc'd return type */
    Type *func_params; /* TY_FUNC only: malloc'd array of param types (nparams long) */
    int   func_nparams;/* TY_FUNC only */
};

static inline Type type_make_int(int width, int is_unsigned) {
    Type t; t.kind = TY_INT; t.width = width; t.is_unsigned = is_unsigned;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = NULL; t.elem_type = NULL; t.length = 0; t.tag = NULL;
    t.func_ret = NULL; t.func_params = NULL; t.func_nparams = 0; return t;
}
static inline Type type_make_bool(void) {
    Type t = type_make_int(1, 1);
    t.is_bool = 1;
    return t;
}
static inline Type type_default_int(void) { return type_make_int(4, 0); }
static inline Type type_make_float(int width) {
    Type t; t.kind = TY_FLOAT; t.width = width; t.is_unsigned = 0;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = NULL; t.elem_type = NULL; t.length = 0; t.tag = NULL;
    t.func_ret = NULL; t.func_params = NULL; t.func_nparams = 0; return t;
}
static inline Type type_make_void(void) {
    Type t; t.kind = TY_VOID; t.width = 0; t.is_unsigned = 0;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = NULL; t.elem_type = NULL; t.length = 0; t.tag = NULL;
    t.func_ret = NULL; t.func_params = NULL; t.func_nparams = 0; return t;
}

/* Deep-clone a Type (recursing into pointee/elem_type). */
Type type_clone(Type t);
/* Free heap-owned sub-types (no-op for TY_INT). Does NOT free `t` itself. */
void type_free(Type *t);
/* Total byte size of a Type: sizeof for scalars, N*elem for arrays,
 * width for structs (which is stashed at parse-time via layout). */
int  type_size(Type t);
int  type_align(Type t); /* natural alignment of a type */
Type type_make_ptr(Type pointee);
Type type_make_array(Type elem, int length);
Type type_make_struct(const char *tag, int size);
Type type_make_func(Type ret, Type * const *params, int nparams);
Type type_decay(Type t);
int  type_is_ptr_or_array(Type t);
Type type_pointee_or_elem(Type t);
int  type_funcs_equal(Type a, Type b);  /* true if ret + all params match */

/* ------------------------------------------------------------------ */
/* Expression — Slice 2 introduces arithmetic expressions              */
/* ------------------------------------------------------------------ */

typedef enum {
    EX_INT_LIT,
    EX_BINOP,
    EX_UNARY,
    EX_VAR,     /* variable reference: name */
    EX_ASSIGN,  /* lvalue = rvalue; result is the assigned value */
    EX_CALL,    /* callee(arg1, arg2, ...) */
    EX_STR,     /* "hello" — anonymous global char array; value is const char* */
    EX_ADDR,    /* &lvalue */
    EX_DEREF,   /* *ptr — lvalue */
    EX_INDEX,   /* a[i]  — lvalue; desugars to *(a+i) in IR */
    EX_MEMBER,  /* s.x   — lvalue; requires s.type to be TY_STRUCT */
    EX_CAST,    /* (T)expr */
    EX_SIZEOF_TYPE,  /* sizeof(T)     — compile-time integer */
    EX_SIZEOF_EXPR,  /* sizeof(expr)  — compile-time integer */
    EX_ALIGNOF_TYPE, /* _Alignof(T)  — compile-time integer (alignment) */
    EX_TERNARY, /* cond ? then : else — right associative, lower than || */
    EX_INC_DEC, /* ++lvalue / --lvalue (prefix or postfix) */
    EX_COMPOUND_ASSIGN, /* lvalue op= rvalue */
    EX_COMMA, /* a, b — evaluate a (discard), result is b */
    EX_INIT_LIST, /* { e1, e2, ... } — array/struct initializer; valid only as decl.init */
    EX_FLOAT_LIT, /* 1.5, 1e10, .5 — double value (width selects float vs double) */
    EX_COMPOUND_LITERAL, /* (Type){ ... } — compound literal; yields a value of the named type */
} ExprKind;

typedef enum {
    BOP_ADD,     /* + */
    BOP_SUB,     /* - */
    BOP_MUL,     /* * */
    BOP_DIV,     /* / */
    BOP_MOD,     /* % */
    BOP_EQ,      /* == */
    BOP_NE,      /* != */
    BOP_LT,      /* <  */
    BOP_LE,      /* <= */
    BOP_GT,      /* >  */
    BOP_GE,      /* >= */
    BOP_AND,     /* && — logical AND, short-circuit */
    BOP_OR,      /* || — logical OR, short-circuit */
    BOP_BITAND,  /* &  — bitwise AND */
    BOP_BITOR,   /* |  — bitwise OR */
    BOP_BITXOR,  /* ^  — bitwise XOR */
    BOP_SHL,     /* << — left shift */
    BOP_SHR,     /* >> — right shift (arithmetic if signed, logical if unsigned) */
} BinOp;

typedef enum {
    UOP_NEG,     /* -x */
    UOP_POS,     /* +x — no-op */
    UOP_BITNOT,  /* ~x — bitwise NOT */
    UOP_NOT,     /* !x — logical NOT */
} UnaryOp;

typedef struct Expr Expr;

/* Argument-list holder used by EX_CALL — grows as arguments are parsed. */
typedef struct {
    Expr **data;    /* Expr* per argument; owns each pointer */
    size_t len;
    size_t cap;
} ExprArray;

struct Expr {
    ExprKind kind;
    SourceLoc loc;
    Type type;   /* populated by sema; default-initialized to int */
    /* EX_CALL only, when callee is the `va_arg` builtin: the requested
     * type T (va_arg's second argument is a type, not an expression).
     * Kept OUT of the union so it never aliases with EX_CALL::args/callee. */
    Type va_arg_type;
    union {
        int int_val;                                   /* EX_INT_LIT */
        struct { BinOp op; Expr *l, *r; } bin;        /* EX_BINOP */
        struct { UnaryOp op; Expr *operand; } un;     /* EX_UNARY */
        struct { char *name; } var;                    /* EX_VAR */
        struct { Expr *lvalue; Expr *rvalue; } assign;/* EX_ASSIGN */
        struct { Expr *callee; ExprArray args; } call;/* EX_CALL — owns callee expr + args */
        struct { char *bytes; int len; } str;         /* EX_STR — bytes owns strdup'd data, len excludes trailing NUL */
        struct { Expr *operand; } addr;                /* EX_ADDR */
        struct { Expr *operand; } deref;               /* EX_DEREF */
        struct { Expr *array;  Expr *index; } idx;    /* EX_INDEX */
        struct { Expr *obj; char *name; } member;     /* EX_MEMBER — s.name (or (*p).name after arrow rewrite) */
        struct { Type target; Expr *operand; } cast;  /* EX_CAST — owns target sub-types */
        struct { Type target; } sizeof_t;              /* EX_SIZEOF_TYPE */
        struct { Expr *operand; } sizeof_e;            /* EX_SIZEOF_EXPR */
        struct { Type target; } alignof_t;             /* EX_ALIGNOF_TYPE */
        struct { Expr *cond; Expr *then; Expr *else_; } tern; /* EX_TERNARY */
        struct { Expr *operand; int is_inc; int is_prefix; } incdec; /* EX_INC_DEC */
        struct { Expr *lvalue; Expr *rvalue; BinOp op; } comp; /* EX_COMPOUND_ASSIGN */
        struct { Expr *lhs; Expr *rhs; } comma; /* EX_COMMA */
        struct { Expr **elements; int num_elements; int *desig_kind; int *desig_index; char **desig_member; } init_list; /* EX_INIT_LIST — owns each element; desig_kind[i]: -1=positional, 0=[index], 1=.member (name in desig_member[i], index in desig_index[i]) */
        char   *float_text;                                    /* EX_FLOAT_LIT — source text (strdup'd), parsed at IR time for precision */
        struct { Type target_type; Expr *init; } compound; /* EX_COMPOUND_LITERAL — target_type owns sub-types; init is an EX_INIT_LIST */
    } u;
};

/* Ownership: Expr uses malloc; tu_free recurses */
Expr *expr_new_int(int v, SourceLoc loc);
Expr *expr_new_binop(BinOp op, Expr *l, Expr *r, SourceLoc loc);
Expr *expr_new_unary(UnaryOp op, Expr *operand, SourceLoc loc);
Expr *expr_new_var(const char *name, SourceLoc loc);
Expr *expr_new_assign(Expr *lvalue, Expr *rvalue, SourceLoc loc);
Expr *expr_new_call(Expr *callee, SourceLoc loc);
Expr *expr_new_str(const char *bytes, int len, SourceLoc loc);
Expr *expr_new_addr(Expr *operand, SourceLoc loc);
Expr *expr_new_deref(Expr *operand, SourceLoc loc);
Expr *expr_new_index(Expr *array, Expr *index, SourceLoc loc);
Expr *expr_new_member(Expr *obj, const char *name, SourceLoc loc);
Expr *expr_new_cast(Type target, Expr *operand, SourceLoc loc);
Expr *expr_new_sizeof_type(Type t, SourceLoc loc);
Expr *expr_new_sizeof_expr(Expr *operand, SourceLoc loc);
Expr *expr_new_alignof_type(Type t, SourceLoc loc);
Expr *expr_new_ternary(Expr *cond, Expr *then, Expr *else_, SourceLoc loc);
Expr *expr_new_inc_dec(Expr *operand, int is_inc, int is_prefix, SourceLoc loc);
Expr *expr_new_compound_assign(Expr *lvalue, Expr *rvalue, BinOp op, SourceLoc loc);
Expr *expr_new_comma(Expr *l, Expr *r, SourceLoc loc);
Expr *expr_new_init_list(Expr **elements, int num_elements, SourceLoc loc);
Expr *expr_new_float_lit(const char *text, int width, SourceLoc loc);
Expr *expr_new_compound_literal(Type target_type, Expr *init, SourceLoc loc);
void  expr_call_push_arg(Expr *e, Expr *arg);   /* takes ownership of arg */
void  expr_call_set_callee(Expr *e, Expr *callee); /* takes ownership, frees old */
void  expr_free(Expr *e);
/* Set an Expr's type, freeing the old (owning) type first; takes ownership of t. */
void  expr_set_type(Expr *e, Type t);

/* ------------------------------------------------------------------ */
/* Statement                                                           */
/* ------------------------------------------------------------------ */

typedef enum {
    ST_DECL,     /* int x;  or  int x = expr;  */
    ST_EXPR,     /* expr;  (typical: x = 5;) */
    ST_RETURN,   /* return expr; */
    ST_IF,       /* if (cond) then_stmt [else else_stmt] */
    ST_WHILE,    /* while (cond) body */
    ST_FOR,      /* for (init? ; cond? ; step?) body — init/step are Stmt* / Expr* */
    ST_BREAK,    /* break; */
    ST_CONTINUE, /* continue; */
    ST_BLOCK,    /* { stmt* } — introduces a new scope */
    ST_DO_WHILE, /* do body while (cond); — body executes at least once */
    ST_GOTO,     /* goto label; */
    ST_LABEL,    /* label: stmt */
    ST_SWITCH,   /* switch (expr) { case/default ... } */
} StmtKind;

typedef struct Stmt Stmt;
typedef struct StmtArray {
    Stmt *data;
    size_t len;
    size_t cap;
} StmtArray;

/* One case arm of a switch.  `is_default` marks the default arm; otherwise
 * `value` is the constant case value.  `stmts` owns the arm's statements. */
typedef struct {
    int is_default;
    int value;          /* case value (valid when !is_default) */
    StmtArray stmts;    /* statements in this arm */
} SwitchCase;

struct Stmt {
    StmtKind kind;
    SourceLoc loc;
    union {
        struct { char *name; Type type; Expr *init; int storage_class; } decl;   /* ST_DECL: init may be NULL; storage_class: 0=default, 1=static, 2=extern */
        Expr *expr;                                 /* ST_EXPR */
        Expr *value;                                /* ST_RETURN */
        struct { Expr *cond; Stmt *then_s; Stmt *else_s; } if_s; /* ST_IF: else_s may be NULL */
        struct { Expr *cond; Stmt *body; } while_s;              /* ST_WHILE */
        struct { Expr *cond; Stmt *body; } do_s;                 /* ST_DO_WHILE */
        /* ST_FOR: init (may be Stmt or NULL), cond (may be NULL), step (may be NULL). */
        struct { Stmt *init; Expr *cond; Expr *step; Stmt *body; } for_s;
        StmtArray block;                             /* ST_BLOCK — owns its statements */
        struct { char *target; } goto_s;             /* ST_GOTO */
        struct { char *name; Stmt *stmt; } label_s;   /* ST_LABEL — owns stmt */
        struct { Expr *cond; SwitchCase *cases; int num_cases; int cap_cases; } switch_s; /* ST_SWITCH */
    } u;
};

void stmt_array_init(StmtArray *a);
void stmt_array_push(StmtArray *a, Stmt s);
void stmt_array_free(StmtArray *a);
void stmt_free(Stmt *s);

/* Heap-allocated statement helpers used by if/while (which own sub-stmts) */
Stmt *stmt_alloc(void);
void  stmt_free_ptr(Stmt *s);

/* Switch helper: append a case arm (default if is_default) to a ST_SWITCH. */
void switch_push_case(Stmt *s, int is_default, int value);

/* ------------------------------------------------------------------ */
/* Function & package declarations                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    char *name;         /* strdup'd */
    Type type;          /* declared parameter type */
    SourceLoc loc;
} Param;

typedef struct {
    Param *data;
    size_t len;
    size_t cap;
} ParamArray;

void param_array_init(ParamArray *a);
void param_array_push(ParamArray *a, const char *name, Type type, SourceLoc loc);
void param_array_free(ParamArray *a);

typedef struct {
    char *name;         /* strdup'd */
    Type ret_type;      /* declared return type */
    ParamArray params;
    StmtArray body;
    SourceLoc loc;
    int    is_variadic; /* 1 = ends with `...` (variadic function) */
    int    is_extern;   /* 1 = declaration only (`extern int f();`), no body */
    int    is_static;   /* 1 = `static` function — LOCAL linkage */
} FunctionDecl;

typedef struct {
    char *name;         /* strdup'd */
    SourceLoc loc;
} PackageDecl;

/* Struct definition: tag + ordered member list.  Each member has a name,
 * a Type (owned), a computed byte offset within the struct, and a size.
 * `bit_width` is 0 for a normal member, or N (1..64) for a bitfield
 * `unsigned x : N;`.  Bitfield members share a storage unit with adjacent
 * bitfields of the same underlying type (tracked by the struct's
 * `bit_cur_unit` / `bit_cur_used` during member layout). */
typedef struct {
    char *name;
    Type type;
    int  offset;
    int  bit_width;     /* 0 = normal member, else bitfield width in bits */
    int  bit_offset;    /* bit position within the unit (0 = LSB); valid when
                         * bit_width > 0.  Codegen loads the unit, shifts right
                         * by bit_offset, and masks to (1<<bit_width)-1. */
} StructMember;

typedef struct {
    char *tag;            /* xstrdup'd */
    int   is_union;       /* 1 = union (members overlap at offset 0) */
    StructMember *members;
    int num_members;
    int cap_members;
    int size;             /* total size in bytes (already aligned) */
    SourceLoc loc;
    /* Bitfield layout state (valid while members are being added).  A run of
     * adjacent bitfields of the same `type` packs into one "unit"; the unit
     * size is the smallest of {1,2,4,8} bytes holding all its bits.  A
     * non-bitfield member (or a type/width change) closes the current unit. */
    int   bf_unit_type;   /* width (bytes) of the current open bitfield unit */
    int   bf_unit_used;   /* bits used in the current open unit */
    int   bf_unit_offset; /* byte offset of the current open unit */
} StructDef;

typedef struct {
    StructDef *data;
    size_t len;
    size_t cap;
} StructRegistry;

void struct_registry_init(StructRegistry *r);
void struct_registry_free(StructRegistry *r);
/* Register a new struct; returns pointer into registry.  Fails/duplicates
 * are caller's problem — sema checks before calling. */
StructDef *struct_registry_add(StructRegistry *r, const char *tag, SourceLoc loc);
/* Find a struct by tag; NULL if absent. */
StructDef *struct_registry_find(StructRegistry *r, const char *tag);
/* Const variant. */
const StructDef *struct_registry_find_c(const StructRegistry *r, const char *tag);
/* Append a member to a struct definition; computes offset (naturally aligned).
 * `bit_width` is 0 for a normal member, or N (1..64) for a bitfield `x : N;`. */
void struct_def_push_member(StructDef *sd, const char *name, Type ty, int bit_width);

typedef struct {
    FunctionDecl *data;
    size_t len;
    size_t cap;
} FunctionArray;

/* Enum constant: a name → int value.  Owned by an EnumDef. */
typedef struct {
    char *name;     /* xstrdup'd */
    int  value;
} EnumConstant;

/* Enum definition: tag + ordered constant list.  `tag` is NULL for anonymous
 * enums (allowed but cannot be used as a type). */
typedef struct {
    char *tag;              /* xstrdup'd, or NULL */
    EnumConstant *constants;
    int num_constants;
    int cap_constants;
    SourceLoc loc;
} EnumDef;

typedef struct {
    EnumDef *data;
    size_t len;
    size_t cap;
} EnumRegistry;

void enum_registry_init(EnumRegistry *r);
void enum_registry_free(EnumRegistry *r);
/* Register a new enum; returns pointer into registry. */
EnumDef *enum_registry_add(EnumRegistry *r, const char *tag, SourceLoc loc);
/* Find an enum by tag; NULL if absent. */
EnumDef *enum_registry_find(EnumRegistry *r, const char *tag);
/* Append a constant to an enum, computing its value: explicit if `has_value`
 * (value given), else prev+1 (or 0 if first). Returns the assigned value. */
int  enum_def_push_constant(EnumDef *ed, const char *name, int has_value,
                            int value, SourceLoc loc);
/* Look up an enum constant by name across ALL enums; returns pointer to the
 * EnumConstant, or NULL if not found. */
const EnumConstant *enum_registry_find_constant(const EnumRegistry *r,
                                                const char *name);

/* Typedef: a name → Type alias.  The aliased Type is fully owned (heap
 * pointee/elem types and strdup'd tag are cloned on insert). */
typedef struct {
    char *name;     /* xstrdup'd */
    Type type;      /* the aliased type, owned */
} TypedefEntry;

typedef struct {
    TypedefEntry *data;
    size_t len;
    size_t cap;
} TypedefRegistry;

void typedef_registry_init(TypedefRegistry *r);
void typedef_registry_free(TypedefRegistry *r);
/* Register a typedef; returns pointer into registry.  Caller must not already
 * have an entry with the same name (sema checks first). */
TypedefEntry *typedef_registry_add(TypedefRegistry *r, const char *name, Type type);
/* Find a typedef by name; NULL if absent. */
const Type *typedef_registry_find(const TypedefRegistry *r, const char *name);

typedef struct {
    PackageDecl package;
    StmtArray globals;   /* ST_DECL at file scope (globals) */
    FunctionArray functions;
    StructRegistry structs;
    EnumRegistry enums;
    TypedefRegistry typedefs;
} TranslationUnit;

void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);

#endif /* FAKECC_AST_H */
