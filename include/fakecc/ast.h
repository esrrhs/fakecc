#ifndef FAKECC_AST_H
#define FAKECC_AST_H

#include "fakecc/token.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Type — 7a: integer types; 7b/c: pointers + arrays                    */
/* ------------------------------------------------------------------ */

typedef enum {
    TY_INT,     /* char/short/int/long × signed/unsigned */
    TY_PTR,     /* T*  — pointee owned via heap allocation */
    TY_ARRAY,   /* T[N] — elem_type owned via heap; length is a positive int */
} TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    int width;         /* TY_INT: 1/2/4/8. TY_PTR: always 8. TY_ARRAY: elem width. */
    int is_unsigned;   /* TY_INT only */
    Type *pointee;     /* TY_PTR only: malloc'd */
    Type *elem_type;   /* TY_ARRAY only: malloc'd */
    int length;        /* TY_ARRAY only */
};

static inline Type type_make_int(int width, int is_unsigned) {
    Type t; t.kind = TY_INT; t.width = width; t.is_unsigned = is_unsigned;
    t.pointee = NULL; t.elem_type = NULL; t.length = 0; return t;
}
static inline Type type_default_int(void) { return type_make_int(4, 0); }

/* Deep-clone a Type (recursing into pointee/elem_type). */
Type type_clone(Type t);
/* Free heap-owned sub-types (no-op for TY_INT). Does NOT free `t` itself. */
void type_free(Type *t);
/* Total byte size of a Type: sizeof for scalars, N*elem for arrays. */
int  type_size(Type t);
/* T*  → owning-Type wrapper. `pointee` is deep-cloned. */
Type type_make_ptr(Type pointee);
/* T[N] — elem is deep-cloned. */
Type type_make_array(Type elem, int length);
/* Array→pointer decay: TY_ARRAY(E) → TY_PTR(E); otherwise identity clone. */
Type type_decay(Type t);
/* True iff t is TY_PTR or TY_ARRAY. */
int  type_is_ptr_or_array(Type t);
/* Element type for a pointer-or-array — cloned. */
Type type_pointee_or_elem(Type t);

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
    EX_CAST,    /* (T)expr */
    EX_SIZEOF_TYPE,  /* sizeof(T)     — compile-time integer */
    EX_SIZEOF_EXPR,  /* sizeof(expr)  — compile-time integer */
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
} BinOp;

typedef enum {
    UOP_NEG,     /* -x */
    UOP_POS,     /* +x — no-op */
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
    union {
        int int_val;                                   /* EX_INT_LIT */
        struct { BinOp op; Expr *l, *r; } bin;        /* EX_BINOP */
        struct { UnaryOp op; Expr *operand; } un;     /* EX_UNARY */
        struct { char *name; } var;                    /* EX_VAR */
        struct { Expr *lvalue; Expr *rvalue; } assign;/* EX_ASSIGN */
        struct { char *callee; ExprArray args; } call;/* EX_CALL — owns callee + args */
        struct { char *bytes; int len; } str;         /* EX_STR — bytes owns strdup'd data, len excludes trailing NUL */
        struct { Expr *operand; } addr;                /* EX_ADDR */
        struct { Expr *operand; } deref;               /* EX_DEREF */
        struct { Expr *array;  Expr *index; } idx;    /* EX_INDEX */
        struct { Type target; Expr *operand; } cast;  /* EX_CAST — owns target sub-types */
        struct { Type target; } sizeof_t;              /* EX_SIZEOF_TYPE */
        struct { Expr *operand; } sizeof_e;            /* EX_SIZEOF_EXPR */
    } u;
};

/* Ownership: Expr uses malloc; tu_free recurses */
Expr *expr_new_int(int v, SourceLoc loc);
Expr *expr_new_binop(BinOp op, Expr *l, Expr *r, SourceLoc loc);
Expr *expr_new_unary(UnaryOp op, Expr *operand, SourceLoc loc);
Expr *expr_new_var(const char *name, SourceLoc loc);
Expr *expr_new_assign(Expr *lvalue, Expr *rvalue, SourceLoc loc);
Expr *expr_new_call(const char *callee, SourceLoc loc);
Expr *expr_new_str(const char *bytes, int len, SourceLoc loc);
Expr *expr_new_addr(Expr *operand, SourceLoc loc);
Expr *expr_new_deref(Expr *operand, SourceLoc loc);
Expr *expr_new_index(Expr *array, Expr *index, SourceLoc loc);
Expr *expr_new_cast(Type target, Expr *operand, SourceLoc loc);
Expr *expr_new_sizeof_type(Type t, SourceLoc loc);
Expr *expr_new_sizeof_expr(Expr *operand, SourceLoc loc);
void  expr_call_push_arg(Expr *e, Expr *arg);   /* takes ownership of arg */
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
    ST_BLOCK,    /* { stmt* } — introduces a new scope */
} StmtKind;

typedef struct Stmt Stmt;
typedef struct StmtArray {
    Stmt *data;
    size_t len;
    size_t cap;
} StmtArray;

struct Stmt {
    StmtKind kind;
    SourceLoc loc;
    union {
        struct { char *name; Type type; Expr *init; } decl;   /* ST_DECL: init may be NULL */
        Expr *expr;                                 /* ST_EXPR */
        Expr *value;                                /* ST_RETURN */
        struct { Expr *cond; Stmt *then_s; Stmt *else_s; } if_s; /* ST_IF: else_s may be NULL */
        struct { Expr *cond; Stmt *body; } while_s;              /* ST_WHILE */
        StmtArray block;                             /* ST_BLOCK — owns its statements */
    } u;
};

void stmt_array_init(StmtArray *a);
void stmt_array_push(StmtArray *a, Stmt s);
void stmt_array_free(StmtArray *a);
void stmt_free(Stmt *s);

/* Heap-allocated statement helpers used by if/while (which own sub-stmts) */
Stmt *stmt_alloc(void);
void  stmt_free_ptr(Stmt *s);

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
} FunctionDecl;

typedef struct {
    char *name;         /* strdup'd */
    SourceLoc loc;
} PackageDecl;

typedef struct {
    FunctionDecl *data;
    size_t len;
    size_t cap;
} FunctionArray;

typedef struct {
    PackageDecl package;
    StmtArray globals;   /* ST_DECL at file scope (globals) */
    FunctionArray functions;
} TranslationUnit;

void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);

#endif /* FAKECC_AST_H */
