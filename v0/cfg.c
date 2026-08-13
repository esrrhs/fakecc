package main;
static int __fakecc_ctzll(unsigned long _v){int c;for(c=0;!(_v&1);c++)_v>>=1;return c;}
static void __fakecc_va_copy(void *dst, void *src){
    char *d = (char*)dst; char *s = (char*)src;
    for(int i = 0; i < 24; i++) d[i] = s[i];
}
typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef struct {
    const char *file;
    int line;
    int col;
} SourceLoc;
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;
void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_append(Buffer *b, const char *s, size_t n);
void buffer_appendf(Buffer *b, const char *fmt, ...);
char *xstrdup(const char *s);
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
void die_at(const char *file, int line, int col, const char *fmt, ...)
    ;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef int bool;
typedef int IRValue;
typedef enum {
    IR_CONST,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_NEG,
    IR_BAND,
    IR_BOR,
    IR_BXOR,
    IR_BNOT,
    IR_SHL,
    IR_SHR,
    IR_EQ,
    IR_NE,
    IR_FADD,
    IR_FSUB,
    IR_FMUL,
    IR_FDIV,
    IR_FCMP,
    IR_SITOFP,
    IR_FPTOSI,
    IR_FPEXT,
    IR_FPTRUNC,
    IR_LT,
    IR_LE,
    IR_GT,
    IR_GE,
    IR_ALLOCA,
    IR_LOAD,
    IR_STORE,
    IR_ADDR,
    IR_LOAD_PTR,
    IR_STORE_PTR,
    IR_COPY,
    IR_LABEL,
    IR_BR,
    IR_CBR,
    IR_PARAM,
    IR_CALL,
    IR_RETURN,
    IR_SEXT,
    IR_ZEXT,
    IR_TRUNC,
    IR_GADDR,
    IR_FADDR,
} IROpcode;
typedef struct {
    IROpcode op;
    IRValue dst;
    IRValue a, b;
    int imm;
    SourceLoc loc;
    char *call_name;
    IRValue call_args[16];
    int call_nargs;
    IRValue call_callee;
    int width;
    int is_unsigned;
    int64_t float_imm;
    int is_float;
    int alloca_bytes;
} IRInst;
typedef struct {
    IRInst *data;
    size_t len;
    size_t cap;
} IRInstArray;
typedef struct {
    char *name;
    IRInstArray insts;
    int next_value_id;
    int next_label_id;
    SourceLoc loc;
    void *ra;
    void *ra_xmm;
    int *value_width;
    int *value_is_unsigned;
    int value_meta_cap;
    int *value_is_float;
    int ret_width;
    int ret_is_unsigned;
    int ret_is_float;
    int ret_is_struct;
    int ret_is_bool;
    int is_variadic;
    int is_static;
    IRValue sret_value;
} IRFunction;
typedef struct {
    IRFunction *data;
    size_t len;
    size_t cap;
} IRFunctionArray;
typedef struct {
    int offset;
    char *sym;
} GlobalFixup;
typedef struct {
    char *name;
    int size;
    char *init_bytes;
    int is_readonly;
    int is_static;
    SourceLoc loc;
    GlobalFixup *fixups;
    int num_fixups, cap_fixups;
} IRGlobal;
typedef struct {
    IRGlobal *data;
    size_t len;
    size_t cap;
} IRGlobalArray;
typedef struct {
    IRFunctionArray functions;
    IRGlobalArray globals;
} IRModule;
void ir_module_init(IRModule *m);
void ir_module_free(IRModule *m);
typedef enum {
    TK_KW_PACKAGE,
    TK_KW_IMPORT,
    TK_KW_VOID,
    TK_KW_INT,
    TK_KW_FLOAT,
    TK_KW_DOUBLE,
    TK_KW_CHAR,
    TK_KW_SHORT,
    TK_KW_LONG,
    TK_KW_SIGNED,
    TK_KW_UNSIGNED,
    TK_KW_SIZEOF,
    TK_KW_RETURN,
    TK_KW_IF,
    TK_KW_ELSE,
    TK_KW_WHILE,
    TK_KW_FOR,
    TK_KW_GOTO,
    TK_KW_SWITCH,
    TK_KW_CASE,
    TK_KW_DEFAULT,
    TK_KW_BREAK,
    TK_KW_CONTINUE,
    TK_KW_CONST,
    TK_KW_UNION,
    TK_KW_DO,
    TK_KW_ENUM,
    TK_KW_STRUCT,
    TK_KW_TYPEDEF,
    TK_KW_STATIC,
    TK_KW_EXTERN,
    TK_KW_BOOL,
    TK_KW_VOLATILE,
    TK_KW_RESTRICT,
    TK_KW_INLINE,
    TK_KW_ALIGNOF,
    TK_KW_LONG_DOUBLE,
    TK_IDENT,
    TK_INT_LITERAL,
    TK_FLOAT_LITERAL,
    TK_CHAR_LITERAL,
    TK_STRING_LITERAL,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_SEMICOLON,
    TK_COMMA,
    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_PERCENT,
    TK_AMP,
    TK_ANDAND,
    TK_OROR,
    TK_BITOR,
    TK_XOR,
    TK_TILDE,
    TK_NOT,
    TK_SHL,
    TK_SHR,
    TK_SHL_EQ,
    TK_SHR_EQ,
    TK_PLUS_EQ,
    TK_MINUS_EQ,
    TK_STAR_EQ,
    TK_SLASH_EQ,
    TK_PERCENT_EQ,
    TK_AMP_EQ,
    TK_BITOR_EQ,
    TK_XOR_EQ,
    TK_INC,
    TK_DEC,
    TK_QUESTION,
    TK_COLON,
    TK_DOT,
    TK_ELLIPSIS,
    TK_ARROW,
    TK_ASSIGN,
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_LE,
    TK_GT,
    TK_GE,
    TK_EOF,
} TokenKind;
typedef struct {
    TokenKind kind;
    char *text;
    SourceLoc loc;
} Token;
typedef struct {
    Token *data;
    size_t len;
    size_t cap;
} TokenArray;
void token_array_init(TokenArray *a);
void token_array_free(TokenArray *a);
void token_array_push(TokenArray *a, Token t);
typedef enum {
    TY_VOID,
    TY_INT,
    TY_FLOAT,
    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
    TY_FUNC,
} TypeKind;
typedef struct Type Type;
struct Type {
    TypeKind kind;
    int width;
    int is_unsigned;
    unsigned is_const : 1;
    unsigned is_volatile : 1;
    unsigned is_restrict : 1;
    unsigned is_bool : 1;
    Type *pointee;
    Type *elem_type;
    int length;
    char *tag;
    Type *func_ret;
    Type *func_params;
    int func_nparams;
};
static inline Type type_make_int(int width, int is_unsigned) {
    Type t; t.kind = TY_INT; t.width = width; t.is_unsigned = is_unsigned;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = ((void*)0); t.elem_type = ((void*)0); t.length = 0; t.tag = ((void*)0);
    t.func_ret = ((void*)0); t.func_params = ((void*)0); t.func_nparams = 0; return t;
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
    t.pointee = ((void*)0); t.elem_type = ((void*)0); t.length = 0; t.tag = ((void*)0);
    t.func_ret = ((void*)0); t.func_params = ((void*)0); t.func_nparams = 0; return t;
}
static inline Type type_make_void(void) {
    Type t; t.kind = TY_VOID; t.width = 0; t.is_unsigned = 0;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = ((void*)0); t.elem_type = ((void*)0); t.length = 0; t.tag = ((void*)0);
    t.func_ret = ((void*)0); t.func_params = ((void*)0); t.func_nparams = 0; return t;
}
Type type_clone(Type t);
void type_free(Type *t);
int type_size(Type t);
int type_align(Type t);
Type type_make_ptr(Type pointee);
Type type_make_array(Type elem, int length);
Type type_make_struct(const char *tag, int size);
Type type_make_func(Type ret, Type * const *params, int nparams);
Type type_decay(Type t);
int type_is_ptr_or_array(Type t);
Type type_pointee_or_elem(Type t);
int type_funcs_equal(Type a, Type b);
typedef enum {
    EX_INT_LIT,
    EX_BINOP,
    EX_UNARY,
    EX_VAR,
    EX_ASSIGN,
    EX_CALL,
    EX_STR,
    EX_ADDR,
    EX_DEREF,
    EX_INDEX,
    EX_MEMBER,
    EX_CAST,
    EX_SIZEOF_TYPE,
    EX_SIZEOF_EXPR,
    EX_ALIGNOF_TYPE,
    EX_TERNARY,
    EX_INC_DEC,
    EX_COMPOUND_ASSIGN,
    EX_COMMA,
    EX_INIT_LIST,
    EX_FLOAT_LIT,
    EX_COMPOUND_LITERAL,
} ExprKind;
typedef enum {
    BOP_ADD,
    BOP_SUB,
    BOP_MUL,
    BOP_DIV,
    BOP_MOD,
    BOP_EQ,
    BOP_NE,
    BOP_LT,
    BOP_LE,
    BOP_GT,
    BOP_GE,
    BOP_AND,
    BOP_OR,
    BOP_BITAND,
    BOP_BITOR,
    BOP_BITXOR,
    BOP_SHL,
    BOP_SHR,
} BinOp;
typedef enum {
    UOP_NEG,
    UOP_POS,
    UOP_BITNOT,
    UOP_NOT,
} UnaryOp;
typedef struct Expr Expr;
int fold_const_int(const Expr *e, long long *out);
typedef struct {
    Expr **data;
    size_t len;
    size_t cap;
} ExprArray;
struct Expr {
    ExprKind kind;
    SourceLoc loc;
    Type type;
    Type va_arg_type;
    union {
        int int_val;
        struct { BinOp op; Expr *l, *r; } bin;
        struct { UnaryOp op; Expr *operand; } un;
        struct { char *name; } var;
        struct { Expr *lvalue; Expr *rvalue; } assign;
        struct { Expr *callee; ExprArray args; } call;
        struct { char *bytes; int len; } str;
        struct { Expr *operand; } addr;
        struct { Expr *operand; } deref;
        struct { Expr *array; Expr *index; } idx;
        struct { Expr *obj; char *name; } member;
        struct { Type target; Expr *operand; } cast;
        struct { Type target; } sizeof_t;
        struct { Expr *operand; } sizeof_e;
        struct { Type target; } alignof_t;
        struct { Expr *cond; Expr *then; Expr *else_; } tern;
        struct { Expr *operand; int is_inc; int is_prefix; } incdec;
        struct { Expr *lvalue; Expr *rvalue; BinOp op; } comp;
        struct { Expr *lhs; Expr *rhs; } comma;
        struct { Expr **elements; int num_elements; int *desig_kind; int *desig_index; char **desig_member; } init_list;
        char *float_text;
        struct { Type target_type; Expr *init; } compound;
    } u;
};
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
void expr_call_push_arg(Expr *e, Expr *arg);
void expr_call_set_callee(Expr *e, Expr *callee);
void expr_free(Expr *e);
void expr_set_type(Expr *e, Type t);
typedef enum {
    ST_DECL,
    ST_EXPR,
    ST_RETURN,
    ST_IF,
    ST_WHILE,
    ST_FOR,
    ST_BREAK,
    ST_CONTINUE,
    ST_BLOCK,
    ST_DO_WHILE,
    ST_GOTO,
    ST_LABEL,
    ST_SWITCH,
} StmtKind;
typedef struct Stmt Stmt;
typedef struct StmtArray {
    Stmt *data;
    size_t len;
    size_t cap;
} StmtArray;
typedef struct {
    int is_default;
    int value;
    StmtArray stmts;
} SwitchCase;
struct Stmt {
    StmtKind kind;
    SourceLoc loc;
    union {
        struct { char *name; Type type; Expr *init; int storage_class; } decl;
        Expr *expr;
        Expr *value;
        struct { Expr *cond; Stmt *then_s; Stmt *else_s; } if_s;
        struct { Expr *cond; Stmt *body; } while_s;
        struct { Expr *cond; Stmt *body; } do_s;
        struct { Stmt *init; Expr *cond; Expr *step; Stmt *body; } for_s;
        StmtArray block;
        struct { char *target; } goto_s;
        struct { char *name; Stmt *stmt; } label_s;
        struct { Expr *cond; SwitchCase *cases; int num_cases; int cap_cases; } switch_s;
    } u;
};
void stmt_array_init(StmtArray *a);
void stmt_array_push(StmtArray *a, Stmt s);
void stmt_array_free(StmtArray *a);
void stmt_free(Stmt *s);
Stmt *stmt_alloc(void);
void stmt_free_ptr(Stmt *s);
void switch_push_case(Stmt *s, int is_default, int value);
typedef struct {
    char *name;
    Type type;
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
    char *name;
    Type ret_type;
    ParamArray params;
    StmtArray body;
    SourceLoc loc;
    int is_variadic;
    int is_extern;
    int is_static;
} FunctionDecl;
typedef struct {
    char *name;
    SourceLoc loc;
} PackageDecl;
typedef struct {
    char *name;
    Type type;
    int offset;
    int bit_width;
    int bit_offset;
} StructMember;
typedef struct {
    char *tag;
    int is_union;
    StructMember *members;
    int num_members;
    int cap_members;
    int size;
    int align;
    SourceLoc loc;
    Type *canonical_type;
    int bf_unit_type;
    int bf_unit_used;
    int bf_unit_offset;
} StructDef;
typedef struct {
    StructDef *data;
    size_t len;
    size_t cap;
} StructRegistry;
void struct_registry_init(StructRegistry *r);
void struct_registry_free(StructRegistry *r);
StructDef *struct_registry_add(StructRegistry *r, const char *tag, SourceLoc loc);
StructDef *struct_registry_find(StructRegistry *r, const char *tag);
const StructDef *struct_registry_find_c(const StructRegistry *r, const char *tag);
void struct_def_push_member(StructDef *sd, const char *name, Type ty, int bit_width);
void struct_def_finish(StructDef *sd);
void struct_def_fixup_self_types(StructDef *sd);
typedef struct {
    FunctionDecl *data;
    size_t len;
    size_t cap;
} FunctionArray;
typedef struct {
    char *name;
    int value;
} EnumConstant;
typedef struct {
    char *tag;
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
EnumDef *enum_registry_add(EnumRegistry *r, const char *tag, SourceLoc loc);
EnumDef *enum_registry_find(EnumRegistry *r, const char *tag);
int enum_def_push_constant(EnumDef *ed, const char *name, int has_value,
                            int value, SourceLoc loc);
const EnumConstant *enum_registry_find_constant(const EnumRegistry *r,
                                                const char *name);
typedef struct {
    char *name;
    Type type;
} TypedefEntry;
typedef struct {
    TypedefEntry *data;
    size_t len;
    size_t cap;
} TypedefRegistry;
void typedef_registry_init(TypedefRegistry *r);
void typedef_registry_free(TypedefRegistry *r);
TypedefEntry *typedef_registry_add(TypedefRegistry *r, const char *name, Type type);
const Type *typedef_registry_find(const TypedefRegistry *r, const char *name);
typedef struct {
    PackageDecl package;
    StmtArray globals;
    FunctionArray functions;
    StructRegistry structs;
    EnumRegistry enums;
    TypedefRegistry typedefs;
} TranslationUnit;
void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);
void ir_generate(const TranslationUnit *tu, IRModule *ir);
const StructRegistry *get_ir_structs(void);
typedef struct {
    int id;
    int label;
    size_t start, end;
    int *preds;
    size_t num_preds;
    int *succs;
    size_t num_succs;
} CFGBlock;
typedef struct {
    CFGBlock *blocks;
    size_t num;
    int entry;
} CFG;
void cfg_build(CFG *g, const IRInstArray *insts);
void cfg_free(CFG *g);
int cfg_find_label(const CFG *g, int label);
void cfg_link(CFG *g, int from, int to);
int *cfg_rpo(const CFG *g);
extern void *malloc(size_t n);
extern void *realloc(void *p, size_t n);
extern void *calloc(size_t n, size_t m);
extern void free(void *p);
extern void exit(int code);
extern void abort(void);
extern int atoi(const char *s);
extern long atol(const char *s);
extern long strtol(const char *s, char **end, int base);
extern double strtod(const char *s, char **end);
extern long double strtold(const char *nptr, char **endptr);
extern void qsort(void *base, size_t n, size_t sz, int (*cmp)(const void*, const void*));
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memmove(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);
extern int memcmp(const void *a, const void *b, size_t n);
extern size_t strlen(const char *s);
extern char *strdup(const char *s);
extern int strcmp(const char *a, const char *b);
extern int strncmp(const char *a, const char *b, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strstr(const char *a, const char *b);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, size_t n);
extern char *strerror(int n);
static void cfg_init(CFG *g) {
    g->blocks = ((void*)0);
    g->num = 0;
    g->entry = 0;
}
static CFGBlock *cfg_new_block(CFG *g, size_t start) {
    g->blocks = xrealloc(g->blocks, (g->num + 1) * sizeof(CFGBlock));
    CFGBlock *b = &g->blocks[g->num];
    b->id = (int)g->num;
    b->label = -1;
    b->start = start;
    b->end = start;
    b->preds = ((void*)0);
    b->num_preds = 0;
    b->succs = ((void*)0);
    b->num_succs = 0;
    g->num++;
    return b;
}
static void block_add_pred(CFGBlock *b, int pred) {
    for (size_t i = 0; i < b->num_preds; i++)
        if (b->preds[i] == pred) return;
    b->preds = xrealloc(b->preds, (b->num_preds + 1) * sizeof(int));
    b->preds[b->num_preds++] = pred;
}
static void block_add_succ(CFGBlock *b, int succ) {
    for (size_t i = 0; i < b->num_succs; i++)
        if (b->succs[i] == succ) return;
    b->succs = xrealloc(b->succs, (b->num_succs + 1) * sizeof(int));
    b->succs[b->num_succs++] = succ;
}
void cfg_link(CFG *g, int from, int to) {
    block_add_succ(&g->blocks[from], to);
    block_add_pred(&g->blocks[to], from);
}
void cfg_free(CFG *g) {
    for (size_t i = 0; i < g->num; i++) {
        free(g->blocks[i].preds);
        free(g->blocks[i].succs);
    }
    free(g->blocks);
    g->blocks = ((void*)0);
    g->num = 0;
}
int cfg_find_label(const CFG *g, int label) {
    for (size_t i = 0; i < g->num; i++)
        if (g->blocks[i].label == label)
            return (int)i;
    return -1;
}
void cfg_build(CFG *g, const IRInstArray *insts) {
    cfg_init(g);
    if (insts->len == 0) {
        cfg_new_block(g, 0);
        return;
    }
    CFGBlock *cur = cfg_new_block(g, 0);
    for (size_t i = 0; i < insts->len; i++) {
        IRInst *inst = &insts->data[i];
        if (inst->op == IR_LABEL) {
            if (cur->end > cur->start) {
                cur = cfg_new_block(g, i);
            }
            cur->label = inst->imm;
            cur->end = i + 1;
            continue;
        }
        cur->end = i + 1;
        if (inst->op == IR_BR || inst->op == IR_CBR || inst->op == IR_RETURN) {
            if (i + 1 < insts->len) {
                cur = cfg_new_block(g, i + 1);
            }
        }
    }
    for (size_t bi = 0; bi < g->num; bi++) {
        CFGBlock *b = &g->blocks[bi];
        if (b->start >= b->end) {
            if (bi + 1 < g->num)
                cfg_link(g, (int)bi, (int)bi + 1);
            continue;
        }
        IRInst *last = &insts->data[b->end - 1];
        switch (last->op) {
        case IR_BR: {
            int t = cfg_find_label(g, last->imm);
            if (t >= 0) cfg_link(g, (int)bi, t);
            break;
        }
        case IR_CBR: {
            int t = cfg_find_label(g, last->imm);
            int f = cfg_find_label(g, last->b);
            if (t >= 0) cfg_link(g, (int)bi, t);
            if (f >= 0) cfg_link(g, (int)bi, f);
            break;
        }
        case IR_RETURN:
            break;
        default:
            if (bi + 1 < g->num)
                cfg_link(g, (int)bi, (int)bi + 1);
            break;
        }
    }
}
int *cfg_rpo(const CFG *g) {
    size_t n = g->num;
    if (n == 0) return ((void*)0);
    char *visited = calloc(n, 1);
    if (!visited) return ((void*)0);
    int *postorder = xmalloc(n * sizeof(int));
    size_t po_len = 0;
    int *stack_blocks = xmalloc(n * sizeof(int));
    size_t *stack_next = xmalloc(n * sizeof(size_t));
    size_t stack_len = 0;
    stack_blocks[0] = g->entry;
    stack_next[0] = 0;
    stack_len = 1;
    visited[g->entry] = 1;
    while (stack_len > 0) {
        int b = stack_blocks[stack_len - 1];
        size_t ci = stack_next[stack_len - 1];
        CFGBlock *blk = &g->blocks[b];
        int found = 0;
        while (ci < blk->num_succs) {
            int s = blk->succs[ci];
            if (!visited[s]) {
                visited[s] = 1;
                stack_next[stack_len - 1] = ci + 1;
                stack_blocks[stack_len] = s;
                stack_next[stack_len] = 0;
                stack_len++;
                found = 1;
                break;
            }
            ci++;
        }
        if (!found) {
            postorder[po_len++] = b;
            stack_len--;
        }
    }
    free(visited);
    free(stack_blocks);
    free(stack_next);
    int *rpo = xmalloc(n * sizeof(int));
    for (size_t i = 0; i < n; i++) rpo[i] = 0x7fffffff;
    for (size_t i = 0; i < po_len; i++)
        rpo[postorder[po_len - 1 - i]] = (int)i;
    free(postorder);
    return rpo;
}
