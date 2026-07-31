#ifndef FAKECC_AST_H
#define FAKECC_AST_H

#include "fakecc/token.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Expression — Slice 2 introduces arithmetic expressions              */
/* ------------------------------------------------------------------ */

typedef enum {
    EX_INT_LIT,
    EX_BINOP,
    EX_UNARY,
    EX_VAR,     /* variable reference: name */
    EX_ASSIGN,  /* lvalue = rvalue; result is the assigned value */
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
struct Expr {
    ExprKind kind;
    SourceLoc loc;
    union {
        int int_val;                                   /* EX_INT_LIT */
        struct { BinOp op; Expr *l, *r; } bin;        /* EX_BINOP */
        struct { UnaryOp op; Expr *operand; } un;     /* EX_UNARY */
        struct { char *name; } var;                    /* EX_VAR */
        struct { Expr *lvalue; Expr *rvalue; } assign;/* EX_ASSIGN */
    } u;
};

/* Ownership: Expr uses malloc; tu_free recurses */
Expr *expr_new_int(int v, SourceLoc loc);
Expr *expr_new_binop(BinOp op, Expr *l, Expr *r, SourceLoc loc);
Expr *expr_new_unary(UnaryOp op, Expr *operand, SourceLoc loc);
Expr *expr_new_var(const char *name, SourceLoc loc);
Expr *expr_new_assign(Expr *lvalue, Expr *rvalue, SourceLoc loc);
void  expr_free(Expr *e);

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
        struct { char *name; Expr *init; } decl;   /* ST_DECL: init may be NULL */
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
    StmtArray body;     /* was: ReturnStmt body; */
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
    FunctionArray functions;
} TranslationUnit;

void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);

#endif /* FAKECC_AST_H */
