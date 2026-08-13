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
struct SourceLoc {
    const char *file;
    int line;
    int col;
};typedef struct SourceLoc SourceLoc;
struct Buffer {
    char *data;
    size_t len;
    size_t cap;
};typedef struct Buffer Buffer;
void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_append(Buffer *b, const char *s, size_t n);
void buffer_appendf(Buffer *b, const char *fmt, ...);
char *xstrdup(const char *s);
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
void die_at(const char *file, int line, int col, const char *fmt, ...);
enum TokenKind {
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
};typedef enum TokenKind TokenKind;
struct Token {
    TokenKind kind;
    char *text;
    SourceLoc loc;
};typedef struct Token Token;
struct TokenArray {
    Token *data;
    size_t len;
    size_t cap;
};typedef struct TokenArray TokenArray;
void token_array_init(TokenArray *a);
void token_array_free(TokenArray *a);
void token_array_push(TokenArray *a, Token t);
enum TypeKind {
    TY_VOID,
    TY_INT,
    TY_FLOAT,
    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
    TY_FUNC,
};typedef enum TypeKind TypeKind;
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
Type type_make_func(Type ret, Type * *params, int nparams);
Type type_decay(Type t);
int type_is_ptr_or_array(Type t);
Type type_pointee_or_elem(Type t);
int type_funcs_equal(Type a, Type b);
enum ExprKind {
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
};typedef enum ExprKind ExprKind;
enum BinOp {
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
};typedef enum BinOp BinOp;
enum UnaryOp {
    UOP_NEG,
    UOP_POS,
    UOP_BITNOT,
    UOP_NOT,
};typedef enum UnaryOp UnaryOp;
typedef struct Expr Expr;
int fold_const_int(const Expr *e, long long *out);
struct ExprArray {
    Expr **data;
    size_t len;
    size_t cap;
};typedef struct ExprArray ExprArray;
union __anon_u_1 {
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
    };struct Expr {union __anon_u_3 {struct __anon_bin_4 { BinOp op; Expr *l, *r; };
struct __anon_un_5 { UnaryOp op; Expr *operand; };
struct __anon_var_6 { char *name; };
struct __anon_assign_7 { Expr *lvalue; Expr *rvalue; };
struct __anon_call_8 { Expr *callee; ExprArray args; };
struct __anon_str_9 { char *bytes; int len; };
struct __anon_addr_10 { Expr *operand; };
struct __anon_deref_11 { Expr *operand; };
struct __anon_idx_12 { Expr *array; Expr *index; };
struct __anon_member_13 { Expr *obj; char *name; };
struct __anon_cast_14 { Type target; Expr *operand; };
struct __anon_sizeof_t_15 { Type target; };
struct __anon_sizeof_e_16 { Expr *operand; };
struct __anon_alignof_t_17 { Type target; };
struct __anon_tern_18 { Expr *cond; Expr *then; Expr *else_; };
struct __anon_incdec_19 { Expr *operand; int is_inc; int is_prefix; };
struct __anon_comp_20 { Expr *lvalue; Expr *rvalue; BinOp op; };
struct __anon_comma_21 { Expr *lhs; Expr *rhs; };
struct __anon_init_list_22 { Expr **elements; int num_elements; int *desig_kind; int *desig_index; char **desig_member; };
struct __anon_compound_23 { Type target_type; Expr *init; };

        int int_val;
        struct __anon_bin_4 bin;
        struct __anon_un_5 un;
        struct __anon_var_6 var;
        struct __anon_assign_7 assign;
        struct __anon_call_8 call;
        struct __anon_str_9 str;
        struct __anon_addr_10 addr;
        struct __anon_deref_11 deref;
        struct __anon_idx_12 idx;
        struct __anon_member_13 member;
        struct __anon_cast_14 cast;
        struct __anon_sizeof_t_15 sizeof_t;
        struct __anon_sizeof_e_16 sizeof_e;
        struct __anon_alignof_t_17 alignof_t;
        struct __anon_tern_18 tern;
        struct __anon_incdec_19 incdec;
        struct __anon_comp_20 comp;
        struct __anon_comma_21 comma;
        struct __anon_init_list_22 init_list;
        char *float_text;
        struct __anon_compound_23 compound;
    };

    ExprKind kind;
    SourceLoc loc;
    Type type;
    Type va_arg_type;
    union __anon_u_3 u;
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
enum StmtKind {
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
};typedef enum StmtKind StmtKind;
typedef struct Stmt Stmt;
struct StmtArray {
    Stmt *data;
    size_t len;
    size_t cap;
};typedef struct StmtArray StmtArray;
struct SwitchCase {
    int is_default;
    int value;
    StmtArray stmts;
};typedef struct SwitchCase SwitchCase;
union __anon_u_2 {
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
    };struct Stmt {union __anon_u_24 {struct __anon_decl_25 { char *name; Type type; Expr *init; int storage_class; };
struct __anon_if_s_26 { Expr *cond; Stmt *then_s; Stmt *else_s; };
struct __anon_while_s_27 { Expr *cond; Stmt *body; };
struct __anon_do_s_28 { Expr *cond; Stmt *body; };
struct __anon_for_s_29 { Stmt *init; Expr *cond; Expr *step; Stmt *body; };
struct __anon_goto_s_30 { char *target; };
struct __anon_label_s_31 { char *name; Stmt *stmt; };
struct __anon_switch_s_32 { Expr *cond; SwitchCase *cases; int num_cases; int cap_cases; };

        struct __anon_decl_25 decl;
        Expr *expr;
        Expr *value;
        struct __anon_if_s_26 if_s;
        struct __anon_while_s_27 while_s;
        struct __anon_do_s_28 do_s;
        struct __anon_for_s_29 for_s;
        StmtArray block;
        struct __anon_goto_s_30 goto_s;
        struct __anon_label_s_31 label_s;
        struct __anon_switch_s_32 switch_s;
    };

    StmtKind kind;
    SourceLoc loc;
    union __anon_u_24 u;
};
void stmt_array_init(StmtArray *a);
void stmt_array_push(StmtArray *a, Stmt s);
void stmt_array_free(StmtArray *a);
void stmt_free(Stmt *s);
Stmt *stmt_alloc(void);
void stmt_free_ptr(Stmt *s);
void switch_push_case(Stmt *s, int is_default, int value);
struct Param {
    char *name;
    Type type;
    SourceLoc loc;
};typedef struct Param Param;
struct ParamArray {
    Param *data;
    size_t len;
    size_t cap;
};typedef struct ParamArray ParamArray;
void param_array_init(ParamArray *a);
void param_array_push(ParamArray *a, const char *name, Type type, SourceLoc loc);
void param_array_free(ParamArray *a);
struct FunctionDecl {
    char *name;
    Type ret_type;
    ParamArray params;
    StmtArray body;
    SourceLoc loc;
    int is_variadic;
    int is_extern;
    int is_static;
};typedef struct FunctionDecl FunctionDecl;
struct PackageDecl {
    char *name;
    SourceLoc loc;
};typedef struct PackageDecl PackageDecl;
struct StructMember {
    char *name;
    Type type;
    int offset;
    int bit_width;
    int bit_offset;
};typedef struct StructMember StructMember;
struct StructDef {
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
};typedef struct StructDef StructDef;
struct StructRegistry {
    StructDef *data;
    size_t len;
    size_t cap;
};typedef struct StructRegistry StructRegistry;
void struct_registry_init(StructRegistry *r);
void struct_registry_free(StructRegistry *r);
StructDef *struct_registry_add(StructRegistry *r, const char *tag, SourceLoc loc);
StructDef *struct_registry_find(StructRegistry *r, const char *tag);
const StructDef *struct_registry_find_c(const StructRegistry *r, const char *tag);
void struct_def_push_member(StructDef *sd, const char *name, Type ty, int bit_width);
void struct_def_finish(StructDef *sd);
void struct_def_fixup_self_types(StructDef *sd);
struct FunctionArray {
    FunctionDecl *data;
    size_t len;
    size_t cap;
};typedef struct FunctionArray FunctionArray;
struct EnumConstant {
    char *name;
    int value;
};typedef struct EnumConstant EnumConstant;
struct EnumDef {
    char *tag;
    EnumConstant *constants;
    int num_constants;
    int cap_constants;
    SourceLoc loc;
};typedef struct EnumDef EnumDef;
struct EnumRegistry {
    EnumDef *data;
    size_t len;
    size_t cap;
};typedef struct EnumRegistry EnumRegistry;
void enum_registry_init(EnumRegistry *r);
void enum_registry_free(EnumRegistry *r);
EnumDef *enum_registry_add(EnumRegistry *r, const char *tag, SourceLoc loc);
EnumDef *enum_registry_find(EnumRegistry *r, const char *tag);
int enum_def_push_constant(EnumDef *ed, const char *name, int has_value,
                            int value, SourceLoc loc);
const EnumConstant *enum_registry_find_constant(const EnumRegistry *r,
                                                const char *name);
struct TypedefEntry {
    char *name;
    Type type;
};typedef struct TypedefEntry TypedefEntry;
struct TypedefRegistry {
    TypedefEntry *data;
    size_t len;
    size_t cap;
};typedef struct TypedefRegistry TypedefRegistry;
void typedef_registry_init(TypedefRegistry *r);
void typedef_registry_free(TypedefRegistry *r);
TypedefEntry *typedef_registry_add(TypedefRegistry *r, const char *name, Type type);
const Type *typedef_registry_find(const TypedefRegistry *r, const char *name);
struct TranslationUnit {
    PackageDecl package;
    StmtArray globals;
    FunctionArray functions;
    StructRegistry structs;
    EnumRegistry enums;
    TypedefRegistry typedefs;
};typedef struct TranslationUnit TranslationUnit;
void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);
typedef struct FILE FILE;
extern FILE *stderr;
extern FILE *stdin;
extern FILE *stdout;
extern int fprintf(FILE *f, const char *fmt, ...);
extern int vfprintf(FILE *f, const char *fmt, va_list ap);
extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t n, const char *fmt, ...);
extern int fputs(const char *s, FILE *f);
extern int fputc(int c, FILE *f);
extern int fflush(FILE *f);
extern FILE *fopen(const char *p, const char *m);
extern int fclose(FILE *f);
extern size_t fwrite(const void *p, size_t n, size_t m, FILE *f);
extern size_t fread(void *p, size_t n, size_t m, FILE *f);
extern void perror(const char *s);
extern int fileno(FILE *f);
extern int fseek(FILE *f, long off, int whence);
extern long ftell(FILE *f);
typedef long fpos_t;
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
Type type_clone(Type t) {
    Type r = t;
    if (t.kind == TY_PTR && t.pointee) {
        if (t.pointee->kind == TY_STRUCT) {
            r.pointee = t.pointee;
        } else {
            r.pointee = malloc(sizeof(Type));
            if (!r.pointee) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            *r.pointee = type_clone(*t.pointee);
        }
    } else {
        r.pointee = ((void*)0);
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
        r.elem_type = ((void*)0);
    }
    r.tag = t.tag ? xstrdup(t.tag) : ((void*)0);
    if (t.kind == TY_FUNC && t.func_ret) {
        if (t.func_ret->kind == TY_STRUCT) {
            r.func_ret = t.func_ret;
        } else {
            r.func_ret = malloc(sizeof(Type));
            if (!r.func_ret) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            *r.func_ret = type_clone(*t.func_ret);
        }
    } else {
        r.func_ret = ((void*)0);
    }
    if (t.kind == TY_FUNC && t.func_nparams > 0 && t.func_params) {
        r.func_params = malloc(t.func_nparams * sizeof(Type));
        if (!r.func_params) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        for (int i = 0; i < t.func_nparams; i++)
            r.func_params[i] = type_clone(t.func_params[i]);
    } else {
        r.func_params = ((void*)0);
    }
    return r;
}
void type_free(Type *t) {
    if (!t) return;
    if (t->pointee) {
        if (t->pointee->kind != TY_STRUCT) {
            type_free(t->pointee); free(t->pointee);
        }
        t->pointee = ((void*)0);
    }
    if (t->elem_type) {
        if (t->elem_type->kind != TY_STRUCT) {
            type_free(t->elem_type); free(t->elem_type);
        }
        t->elem_type = ((void*)0);
    }
    if (t->tag) { free(t->tag); t->tag = ((void*)0); }
    if (t->func_ret) {
        if (t->func_ret->kind != TY_STRUCT) {
            type_free(t->func_ret); free(t->func_ret);
        }
        t->func_ret = ((void*)0);
    }
    if (t->func_params) {
        for (int i = 0; i < t->func_nparams; i++) type_free(&t->func_params[i]);
        free(t->func_params); t->func_params = ((void*)0);
    }
}
extern const StructRegistry *get_ir_structs(void);
int type_size(Type t) {
    switch (t.kind) {
    case TY_VOID: return 0;
    case TY_INT: return t.width;
    case TY_FLOAT: return t.width;
    case TY_PTR: return 8;
    case TY_ARRAY: return type_size(*t.elem_type) * t.length;
    case TY_STRUCT: {
        if (t.tag) {
            const StructRegistry *reg = get_ir_structs();
            if (reg) {
                const StructDef *sd = struct_registry_find_c(reg, t.tag);
                if (sd && sd->size > 0) return sd->size;
            }
        }
        return t.width;
    }
    case TY_FUNC: return 0;
    }
    return 0;
}
Type type_make_ptr(Type pointee) {
    Type t; t.kind = TY_PTR; t.width = 8; t.is_unsigned = 1;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.elem_type = ((void*)0); t.length = 0; t.tag = ((void*)0);
    t.func_ret = ((void*)0); t.func_params = ((void*)0); t.func_nparams = 0;
    t.pointee = malloc(sizeof(Type));
    if (!t.pointee) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *t.pointee = type_clone(pointee);
    return t;
}
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
    t.pointee = ((void*)0); t.tag = ((void*)0);
    t.func_ret = ((void*)0); t.func_params = ((void*)0); t.func_nparams = 0;
    t.elem_type = malloc(sizeof(Type));
    if (!t.elem_type) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    *t.elem_type = type_clone(elem);
    return t;
}
Type type_make_struct(const char *tag, int size) {
    Type t; t.kind = TY_STRUCT; t.width = size; t.is_unsigned = 0;
    t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = ((void*)0); t.elem_type = ((void*)0); t.length = 0;
    t.func_ret = ((void*)0); t.func_params = ((void*)0); t.func_nparams = 0;
    t.tag = xstrdup(tag);
    return t;
}
Type type_make_func(Type ret, Type * *params, int nparams) {
    Type t; t.kind = TY_FUNC; t.width = 0; t.is_unsigned = 0; t.is_const = 0; t.is_volatile = 0; t.is_restrict = 0; t.is_bool = 0;
    t.pointee = ((void*)0); t.elem_type = ((void*)0); t.length = 0; t.tag = ((void*)0);
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
        t.func_params = ((void*)0);
    }
    return t;
}
static int types_equal(Type a, Type b);
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
    case TY_FLOAT: return a.width == b.width;
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
    if (t.kind == TY_PTR) return type_clone(*t.pointee);
    if (t.kind == TY_ARRAY) return type_clone(*t.elem_type);
    return type_default_int();
}
void expr_set_type(Expr *e, Type t) {
    if (!e) { type_free(&t); return; }
    type_free(&e->type);
    e->type = t;
}
void struct_registry_init(StructRegistry *r) {
    r->data = ((void*)0); r->len = 0; r->cap = 0;
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
    r->data = ((void*)0); r->len = 0; r->cap = 0;
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
    sd->members = ((void*)0); sd->num_members = 0; sd->cap_members = 0;
    sd->size = 0; sd->align = 1; sd->loc = loc;
    sd->bf_unit_type = 0; sd->bf_unit_used = 0; sd->bf_unit_offset = 0;
    return sd;
}
StructDef *struct_registry_find(StructRegistry *r, const char *tag) {
    for (size_t i = 0; i < r->len; i++)
        if (strcmp(r->data[i].tag, tag) == 0) return &r->data[i];
    return ((void*)0);
}
const StructDef *struct_registry_find_c(const StructRegistry *r, const char *tag) {
    return struct_registry_find((StructRegistry *)r, tag);
}
static int align_up(int x, int align) {
    if (align <= 1) return x;
    return (x + align - 1) & ~(align - 1);
}
int type_align(Type t) {
    switch (t.kind) {
    case TY_VOID: return 1;
    case TY_INT: return t.width;
    case TY_FLOAT: return t.width;
    case TY_PTR: return 8;
    case TY_ARRAY: return type_align(*t.elem_type);
    case TY_STRUCT: return 8;
    case TY_FUNC: return 1;
    }
    return 1;
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
    if (a > sd->align) sd->align = a;
    int off;
    if (sd->is_union) {
        off = 0;
    } else if (bit_width > 0 && ty.kind == TY_INT) {
        int unit_bits = sz * 8;
        if (sd->bf_unit_type == sz && sd->bf_unit_used + bit_width <= unit_bits) {
            off = sd->bf_unit_offset;
        } else {
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
        int unit_end = sd->bf_unit_offset + sz;
        if (unit_end > sd->size) sd->size = unit_end;
        sd->members[sd->num_members].name = xstrdup(name);
        sd->members[sd->num_members].type = type_clone(ty);
        sd->num_members++;
        return;
    } else {
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
        if (sz > sd->size) sd->size = sz;
    } else {
        sd->size = off + sz;
    }
}
void struct_def_finish(StructDef *sd) {
    sd->size = align_up(sd->size, sd->align);
}
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
void enum_registry_init(EnumRegistry *r) {
    r->data = ((void*)0); r->len = 0; r->cap = 0;
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
    r->data = ((void*)0); r->len = 0; r->cap = 0;
}
EnumDef *enum_registry_add(EnumRegistry *r, const char *tag, SourceLoc loc) {
    if (r->len >= r->cap) {
        size_t nc = r->cap ? r->cap * 2 : 4;
        r->data = realloc(r->data, nc * sizeof(EnumDef));
        if (!r->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        r->cap = nc;
    }
    EnumDef *ed = &r->data[r->len++];
    ed->tag = tag ? xstrdup(tag) : ((void*)0);
    ed->constants = ((void*)0); ed->num_constants = 0; ed->cap_constants = 0;
    ed->loc = loc;
    return ed;
}
EnumDef *enum_registry_find(EnumRegistry *r, const char *tag) {
    if (!tag) return ((void*)0);
    for (size_t i = 0; i < r->len; i++)
        if (r->data[i].tag && strcmp(r->data[i].tag, tag) == 0) return &r->data[i];
    return ((void*)0);
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
    return ((void*)0);
}
void typedef_registry_init(TypedefRegistry *r) {
    r->data = ((void*)0); r->len = 0; r->cap = 0;
}
void typedef_registry_free(TypedefRegistry *r) {
    for (size_t i = 0; i < r->len; i++) {
        free(r->data[i].name);
        type_free(&r->data[i].type);
    }
    free(r->data);
    r->data = ((void*)0); r->len = 0; r->cap = 0;
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
    e->type = type;
    return e;
}
const Type *typedef_registry_find(const TypedefRegistry *r, const char *name) {
    for (size_t i = 0; i < r->len; i++)
        if (strcmp(r->data[i].name, name) == 0) return &r->data[i].type;
    return ((void*)0);
}
Expr *expr_new_int(int v, SourceLoc loc) {
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
    e->u.call.callee = callee;
    e->u.call.args.data = ((void*)0);
    e->u.call.args.len = 0;
    e->u.call.args.cap = 0;
    return e;
}
void expr_call_set_callee(Expr *e, Expr *callee) {
    if (!e || e->kind != EX_CALL) return;
    if (e->u.call.callee) expr_free(e->u.call.callee);
    e->u.call.callee = callee;
}
Expr *expr_new_str(const char *bytes, int len, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    e->kind = EX_STR;
    e->loc = loc;
    e->type = type_default_int();
    e->u.str.bytes = malloc(len + 1);
    if (!e->u.str.bytes) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    memcpy(e->u.str.bytes, bytes, len);
    e->u.str.bytes[len] = '\0';
    e->u.str.len = len;
    return e;
}
static Expr *expr_alloc(ExprKind k, SourceLoc loc);
Expr *expr_new_float_lit(const char *text, int width, SourceLoc loc) {
    Expr *e = expr_alloc(EX_FLOAT_LIT, loc);
    e->u.float_text = xstrdup(text);
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
    e->u.compound.target_type = type_clone(target_type);
    e->u.compound.init = init;
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
    case EX_ADDR: expr_free(e->u.addr.operand); break;
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
    }
    type_free(&e->type);
    free(e);
}
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
    a->data = ((void*)0);
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
    a->data = ((void*)0);
    a->len = 0;
    a->cap = 0;
}
void tu_init(TranslationUnit *tu) {
    tu->package.name = ((void*)0);
    tu->package.loc.file = ((void*)0);
    tu->package.loc.line = 0;
    tu->package.loc.col = 0;
    stmt_array_init(&tu->globals);
    tu->functions.data = ((void*)0);
    tu->functions.len = 0;
    tu->functions.cap = 0;
    struct_registry_init(&tu->structs);
    enum_registry_init(&tu->enums);
    typedef_registry_init(&tu->typedefs);
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
    a->data = ((void*)0); a->len = 0; a->cap = 0;
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
    a->data = ((void*)0); a->len = 0; a->cap = 0;
}
int fold_const_int(const Expr *e, long long *out) {
    if (!e) return 0;
    if (e->kind == EX_INT_LIT) {
        *out = e->u.int_val;
        return 1;
    }
    if (e->kind == EX_CAST) {
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
long long l;
long long r;
        if (!fold_const_int(e->u.bin.l, &l)) return 0;
        if (!fold_const_int(e->u.bin.r, &r)) return 0;
        switch (e->u.bin.op) {
        case BOP_ADD: *out = l + r; return 1;
        case BOP_SUB: *out = l - r; return 1;
        case BOP_MUL: *out = l * r; return 1;
        case BOP_DIV: if (r == 0) { *out = 0; return 1; } *out = l / r; return 1;
        case BOP_MOD: if (r == 0) { *out = 0; return 1; } *out = l % r; return 1;
        case BOP_BITAND: *out = l & r; return 1;
        case BOP_BITOR: *out = l | r; return 1;
        case BOP_BITXOR: *out = l ^ r; return 1;
        case BOP_SHL: *out = l << r; return 1;
        case BOP_SHR: *out = l >> r; return 1;
        case BOP_EQ: *out = (l == r) ? 1 : 0; return 1;
        case BOP_NE: *out = (l != r) ? 1 : 0; return 1;
        case BOP_LT: *out = (l < r) ? 1 : 0; return 1;
        case BOP_LE: *out = (l <= r) ? 1 : 0; return 1;
        case BOP_GT: *out = (l > r) ? 1 : 0; return 1;
        case BOP_GE: *out = (l >= r) ? 1 : 0; return 1;
        default: return 0;
        }
    }
    return 0;
}
