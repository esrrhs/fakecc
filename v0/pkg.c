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
enum SysVRegClass {
    SYSV_CLS_INTEGER = 1,
    SYSV_CLS_SSE = 2
};typedef enum SysVRegClass SysVRegClass;
int sysv_classify_agg(Type t, SysVRegClass cls[2]);
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
        long long int_val;
        struct { BinOp op; Expr *l, *r; } bin;
        struct { UnaryOp op; Expr *operand; } un;
        struct { char *name; char *pkg; } var;
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
struct __anon_var_6 { char *name; char *pkg; };
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

        long long int_val;
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
Expr *expr_new_int(long long v, SourceLoc loc);
Expr *expr_new_int_typed(long long v, int width, int is_unsigned, SourceLoc loc);
Expr *expr_new_binop(BinOp op, Expr *l, Expr *r, SourceLoc loc);
Expr *expr_new_unary(UnaryOp op, Expr *operand, SourceLoc loc);
Expr *expr_new_var(const char *name, SourceLoc loc);
Expr *expr_new_var_qual(const char *pkg, const char *name, SourceLoc loc);
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
struct ImportDecl {
    char *name;
    SourceLoc loc;
};typedef struct ImportDecl ImportDecl;
struct ImportArray {
    ImportDecl *data;
    size_t len;
    size_t cap;
};typedef struct ImportArray ImportArray;
void import_array_init(ImportArray *a);
void import_array_push(ImportArray *a, const char *name, SourceLoc loc);
void import_array_free(ImportArray *a);
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
    ImportArray imports;
    StmtArray globals;
    FunctionArray functions;
    StructRegistry structs;
    EnumRegistry enums;
    TypedefRegistry typedefs;
};typedef struct TranslationUnit TranslationUnit;
void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);
typedef struct Package Package;
typedef struct PkgContext PkgContext;
struct PkgFuncExport {
    char *name;
    Type ret_type;
    Type param_types[16];
    int arity;
    int is_variadic;
    int is_extern;
    SourceLoc loc;
    TranslationUnit *tu;
};typedef struct PkgFuncExport PkgFuncExport;
struct PkgGlobalExport {
    char *name;
    Type type;
    int is_extern;
    SourceLoc loc;
    TranslationUnit *tu;
};typedef struct PkgGlobalExport PkgGlobalExport;
struct Package {
    char *name;
    char *dir;
    TranslationUnit *files;
    size_t nfiles;
    int owns_files;
    PkgFuncExport *funcs;
    size_t nfuncs;
    PkgGlobalExport *globals;
    size_t nglobals;
    TypedefRegistry typedefs;
    StructRegistry structs;
    EnumRegistry enums;
};
struct PkgContext {
    char **search_paths;
    int npaths;
    Package **pkgs;
    size_t npkgs;
    size_t cap_pkgs;
    char **loading;
    size_t nloading;
    size_t cap_loading;
};
void pkg_ctx_init(PkgContext *ctx);
void pkg_ctx_free(PkgContext *ctx);
void pkg_ctx_add_path(PkgContext *ctx, const char *dir);
Package *pkg_load(PkgContext *ctx, const char *name, SourceLoc loc);
Package *pkg_register_tus(PkgContext *ctx, const char *name,
                          TranslationUnit **tus, size_t ntus);
Package *pkg_find(const PkgContext *ctx, const char *name);
const PkgFuncExport *pkg_find_func(const Package *pkg, const char *name);
const PkgGlobalExport *pkg_find_global(const Package *pkg, const char *name);
const Type *pkg_find_typedef(const Package *pkg, const char *name);
const StructDef *pkg_find_struct(const Package *pkg, const char *name);
const EnumDef *pkg_find_enum(const Package *pkg, const char *name);
const EnumConstant *pkg_find_enum_const(const Package *pkg, const char *name);
void pkg_clone_struct_into(StructRegistry *dst, const StructDef *src);
void pkg_import_typedef(TranslationUnit *tu, const char *name, const Type *src,
                        const Package *pkg);
const char *pkg_suggest_export(const PkgContext *ctx, const char *name);
void lex(const char *source, const char *filename, TokenArray *out);
struct PkgContext;
void parse(const TokenArray *tokens, TranslationUnit *tu);
void parse_in_pkg(const TokenArray *tokens, TranslationUnit *tu,
                  struct PkgContext *ctx);
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
extern int puts(const char *s);
extern int putchar(int c);
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
extern unsigned long strtoul(const char *s, char **end, int base);
extern unsigned long long strtoull(const char *s, char **end, int base);
extern double strtod(const char *s, char **end);
extern float strtof(const char *s, char **end);
extern long double strtold(const char *nptr, char **endptr);
extern void qsort(void *base, size_t n, size_t sz, int (*cmp)(const void*, const void*));
extern char *getenv(const char *name);
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
static long pkg_open(const char *path, long flags) {
    return __syscall(2, (long)path, flags, 0);
}
static long pkg_close(long fd) { return __syscall(3, fd); }
static long pkg_getdents(long fd, void *buf, long n) {
    return __syscall(217, fd, (long)buf, n);
}
struct pkg_dirent64 {
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1];
};
void pkg_ctx_init(PkgContext *ctx) {
    memset(ctx, 0, sizeof(*ctx));
}
void pkg_ctx_free(PkgContext *ctx) {
    for (int i = 0; i < ctx->npaths; i++) free(ctx->search_paths[i]);
    free(ctx->search_paths);
    for (size_t i = 0; i < ctx->npkgs; i++) {
        Package *p = ctx->pkgs[i];
        free(p->name);
        free(p->dir);
        if (p->owns_files) {
            for (size_t f = 0; f < p->nfiles; f++)
                tu_free(&p->files[f]);
        }
        free(p->files);
        for (size_t j = 0; j < p->nfuncs; j++) {
            free(p->funcs[j].name);
            type_free(&p->funcs[j].ret_type);
            for (int k = 0; k < p->funcs[j].arity && k < 16; k++)
                type_free(&p->funcs[j].param_types[k]);
        }
        free(p->funcs);
        for (size_t j = 0; j < p->nglobals; j++) {
            free(p->globals[j].name);
            type_free(&p->globals[j].type);
        }
        free(p->globals);
        typedef_registry_free(&p->typedefs);
        struct_registry_free(&p->structs);
        enum_registry_free(&p->enums);
        free(p);
    }
    free(ctx->pkgs);
    for (size_t i = 0; i < ctx->nloading; i++) free(ctx->loading[i]);
    free(ctx->loading);
    memset(ctx, 0, sizeof(*ctx));
}
void pkg_ctx_add_path(PkgContext *ctx, const char *dir) {
    if (!dir || !dir[0]) return;
    for (int i = 0; i < ctx->npaths; i++)
        if (strcmp(ctx->search_paths[i], dir) == 0) return;
    ctx->search_paths = xrealloc(ctx->search_paths,
                                 (size_t)(ctx->npaths + 1) * sizeof(char *));
    ctx->search_paths[ctx->npaths++] = xstrdup(dir);
}
Package *pkg_find(const PkgContext *ctx, const char *name) {
    for (size_t i = 0; i < ctx->npkgs; i++)
        if (strcmp(ctx->pkgs[i]->name, name) == 0) return ctx->pkgs[i];
    return ((void*)0);
}
const PkgFuncExport *pkg_find_func(const Package *pkg, const char *name) {
    for (size_t i = 0; i < pkg->nfuncs; i++)
        if (strcmp(pkg->funcs[i].name, name) == 0) return &pkg->funcs[i];
    return ((void*)0);
}
const PkgGlobalExport *pkg_find_global(const Package *pkg, const char *name) {
    for (size_t i = 0; i < pkg->nglobals; i++)
        if (strcmp(pkg->globals[i].name, name) == 0) return &pkg->globals[i];
    return ((void*)0);
}
const Type *pkg_find_typedef(const Package *pkg, const char *name) {
    return typedef_registry_find(&pkg->typedefs, name);
}
const StructDef *pkg_find_struct(const Package *pkg, const char *name) {
    return struct_registry_find_c(&pkg->structs, name);
}
const EnumDef *pkg_find_enum(const Package *pkg, const char *name) {
    return enum_registry_find((EnumRegistry *)&pkg->enums, name);
}
const EnumConstant *pkg_find_enum_const(const Package *pkg, const char *name) {
    return enum_registry_find_constant(&pkg->enums, name);
}
const char *pkg_suggest_export(const PkgContext *ctx, const char *name) {
    for (size_t i = 0; i < ctx->npkgs; i++) {
        Package *p = ctx->pkgs[i];
        if (pkg_find_func(p, name) || pkg_find_global(p, name)
            || pkg_find_typedef(p, name))
            return p->name;
    }
    return ((void*)0);
}
void pkg_clone_struct_into(StructRegistry *dst, const StructDef *src) {
    StructDef *exist = struct_registry_find(dst, src->tag);
    if (exist) {
        if (exist->num_members == src->num_members
            && exist->size == src->size
            && exist->is_union == src->is_union)
            return;
        die_at(src->loc.file, src->loc.line, src->loc.col,
               "conflicting definitions of '%s%s'",
               src->is_union ? "union " : "struct ", src->tag);
    }
    StructDef *sd = struct_registry_add(dst, src->tag, src->loc);
    sd->is_union = src->is_union;
    for (int i = 0; i < src->num_members; i++) {
        struct_def_push_member(sd, src->members[i].name,
                               type_clone(src->members[i].type),
                               src->members[i].bit_width);
        if (src->members[i].bit_width > 0) {
            sd->members[sd->num_members - 1].offset = src->members[i].offset;
            sd->members[sd->num_members - 1].bit_offset = src->members[i].bit_offset;
        }
    }
    sd->size = src->size;
    sd->align = src->align;
    if (sd->canonical_type) {
        sd->canonical_type->width = sd->size;
    }
}
void pkg_import_typedef(TranslationUnit *tu, const char *name, const Type *src,
                        const Package *pkg) {
    const Type *exist = typedef_registry_find(&tu->typedefs, name);
    if (exist) return;
    Type t = type_clone(*src);
    if (t.kind == TY_STRUCT && t.tag) {
        const StructDef *sd = pkg_find_struct(pkg, t.tag);
        if (!sd)
            sd = struct_registry_find_c(&tu->structs, t.tag);
        if (sd)
            pkg_clone_struct_into(&tu->structs, sd);
        StructDef *local = struct_registry_find(&tu->structs, t.tag);
        if (local) t.width = local->size;
    } else if (t.kind == TY_PTR && t.pointee && t.pointee->kind == TY_STRUCT
               && t.pointee->tag) {
        const StructDef *sd = pkg_find_struct(pkg, t.pointee->tag);
        if (sd)
            pkg_clone_struct_into(&tu->structs, sd);
    }
    typedef_registry_add(&tu->typedefs, name, t);
}
static char *path_join(const char *a, const char *b) {
    size_t na = strlen(a), nb = strlen(b);
    int slash = (na > 0 && a[na - 1] != '/');
    char *p = xmalloc(na + (size_t)slash + nb + 1);
    memcpy(p, a, na);
    if (slash) p[na++] = '/';
    memcpy(p + na, b, nb + 1);
    return p;
}
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot open '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, 2);
    long size = ftell(f);
    fseek(f, 0, 0);
    char *buf = xmalloc((size_t)size + 1);
    size_t nread = fread(buf, 1, (size_t)size, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}
static int ends_with_c(const char *name) {
    size_t n = strlen(name);
    return n >= 2 && name[n - 2] == '.' && name[n - 1] == 'c';
}
static int is_dir(const char *path) {
    long fd = pkg_open(path, 0 | 65536);
    if (fd < 0) return 0;
    pkg_close(fd);
    return 1;
}
static int dir_has_c(const char *dir) {
    long fd = pkg_open(dir, 0 | 65536);
    if (fd < 0) return 0;
    char buf[2048];
    int found = 0;
    for (;;) {
        long nread = pkg_getdents(fd, buf, (long)sizeof buf);
        if (nread <= 0) break;
        long off = 0;
        while (off < nread) {
            struct pkg_dirent64 *e = (struct pkg_dirent64 *)(buf + off);
            if (ends_with_c(e->d_name)) { found = 1; break; }
            off = off + e->d_reclen;
        }
        if (found) break;
    }
    pkg_close(fd);
    return found;
}
static char *find_pkg_dir(PkgContext *ctx, const char *name) {
    for (int i = 0; i < ctx->npaths; i++) {
        char *cand = path_join(ctx->search_paths[i], name);
        if (is_dir(cand) && dir_has_c(cand))
            return cand;
        free(cand);
    }
    return ((void*)0);
}
static void loading_push(PkgContext *ctx, const char *name) {
    if (ctx->nloading >= ctx->cap_loading) {
        ctx->cap_loading = ctx->cap_loading ? ctx->cap_loading * 2 : 4;
        ctx->loading = xrealloc(ctx->loading, ctx->cap_loading * sizeof(char *));
    }
    ctx->loading[ctx->nloading++] = xstrdup(name);
}
static void loading_pop(PkgContext *ctx) {
    if (ctx->nloading == 0) return;
    free(ctx->loading[--ctx->nloading]);
}
static int loading_contains(const PkgContext *ctx, const char *name) {
    for (size_t i = 0; i < ctx->nloading; i++)
        if (strcmp(ctx->loading[i], name) == 0) return 1;
    return 0;
}
static void pkgs_push(PkgContext *ctx, Package *p) {
    if (ctx->npkgs >= ctx->cap_pkgs) {
        ctx->cap_pkgs = ctx->cap_pkgs ? ctx->cap_pkgs * 2 : 4;
        ctx->pkgs = xrealloc(ctx->pkgs, ctx->cap_pkgs * sizeof(Package *));
    }
    ctx->pkgs[ctx->npkgs++] = p;
}
static char **list_c_files(const char *dir, size_t *nout) {
    long fd = pkg_open(dir, 0 | 65536);
    if (fd < 0) {
        fprintf(stderr, "fakecc: cannot open package directory '%s'\n", dir);
        exit(1);
    }
    char **names = ((void*)0);
    size_t n = 0, cap = 0;
    char buf[2048];
    for (;;) {
        long nread = pkg_getdents(fd, buf, (long)sizeof buf);
        if (nread <= 0) break;
        long off = 0;
        while (off < nread) {
            struct pkg_dirent64 *e = (struct pkg_dirent64 *)(buf + off);
            if (ends_with_c(e->d_name)) {
                if (n >= cap) {
                    cap = cap ? cap * 2 : 4;
                    names = xrealloc(names, cap * sizeof(char *));
                }
                names[n++] = xstrdup(e->d_name);
            }
            off = off + e->d_reclen;
        }
    }
    pkg_close(fd);
    for (size_t i = 1; i < n; i++) {
        char *key = names[i];
        size_t j = i;
        while (j > 0 && strcmp(names[j - 1], key) > 0) {
            names[j] = names[j - 1];
            j--;
        }
        names[j] = key;
    }
    *nout = n;
    return names;
}
static void build_exports(Package *pkg) {
    typedef_registry_init(&pkg->typedefs);
    struct_registry_init(&pkg->structs);
    enum_registry_init(&pkg->enums);
    pkg->funcs = ((void*)0);
    pkg->nfuncs = 0;
    pkg->globals = ((void*)0);
    pkg->nglobals = 0;
    for (size_t f = 0; f < pkg->nfiles; f++) {
        TranslationUnit *tu = &pkg->files[f];
        for (size_t i = 0; i < tu->functions.len; i++) {
            FunctionDecl *fn = &tu->functions.data[i];
            if (fn->is_static) continue;
            if (pkg_find_func(pkg, fn->name)) continue;
            pkg->funcs = xrealloc(pkg->funcs,
                                  (pkg->nfuncs + 1) * sizeof(PkgFuncExport));
            PkgFuncExport *e = &pkg->funcs[pkg->nfuncs++];
            memset(e, 0, sizeof(*e));
            e->name = xstrdup(fn->name);
            e->ret_type = type_clone(fn->ret_type);
            e->arity = (int)fn->params.len;
            e->is_variadic = fn->is_variadic;
            e->is_extern = fn->is_extern;
            e->loc = fn->loc;
            e->tu = tu;
            for (int k = 0; k < e->arity && k < 16; k++)
                e->param_types[k] = type_clone(fn->params.data[k].type);
        }
        for (size_t i = 0; i < tu->globals.len; i++) {
            Stmt *s = &tu->globals.data[i];
            if (s->kind != ST_DECL) continue;
            if (s->u.decl.storage_class == 1) continue;
            if (pkg_find_global(pkg, s->u.decl.name)) continue;
            pkg->globals = xrealloc(pkg->globals,
                                    (pkg->nglobals + 1) * sizeof(PkgGlobalExport));
            PkgGlobalExport *e = &pkg->globals[pkg->nglobals++];
            memset(e, 0, sizeof(*e));
            e->name = xstrdup(s->u.decl.name);
            e->type = type_clone(s->u.decl.type);
            e->is_extern = (s->u.decl.storage_class == 2);
            e->loc = s->loc;
            e->tu = tu;
        }
        for (size_t i = 0; i < tu->typedefs.len; i++) {
            TypedefEntry *te = &tu->typedefs.data[i];
            if (strcmp(te->name, "va_list") == 0) continue;
            if (typedef_registry_find(&pkg->typedefs, te->name)) continue;
            typedef_registry_add(&pkg->typedefs, te->name, type_clone(te->type));
        }
        for (size_t i = 0; i < tu->structs.len; i++) {
            StructDef *sd = &tu->structs.data[i];
            if (strcmp(sd->tag, "__va_list_tag") == 0) continue;
            if (sd->tag && strncmp(sd->tag, "__anon_", 7) == 0) continue;
            if (struct_registry_find(&pkg->structs, sd->tag)) continue;
            pkg_clone_struct_into(&pkg->structs, sd);
        }
        for (size_t i = 0; i < tu->enums.len; i++) {
            EnumDef *ed = &tu->enums.data[i];
            if (!ed->tag) continue;
            if (ed->tag && strncmp(ed->tag, "__anon_", 7) == 0) continue;
            if (enum_registry_find(&pkg->enums, ed->tag)) continue;
            EnumDef *ne = enum_registry_add(&pkg->enums, ed->tag, ed->loc);
            for (int c = 0; c < ed->num_constants; c++)
                enum_def_push_constant(ne, ed->constants[c].name, 1,
                                       ed->constants[c].value, ed->loc);
        }
    }
}
Package *pkg_load(PkgContext *ctx, const char *name, SourceLoc loc) {
    if (loading_contains(ctx, name)) {
        die_at(loc.file, loc.line, loc.col,
               "import cycle involving package '%s'", name);
    }
    Package *cached = pkg_find(ctx, name);
    if (cached) return cached;
    char *dir = find_pkg_dir(ctx, name);
    if (!dir) {
        die_at(loc.file, loc.line, loc.col,
               "package '%s' not found (search path has %d entries)",
               name, ctx->npaths);
    }
    loading_push(ctx, name);
    size_t nnames = 0;
    char **names = list_c_files(dir, &nnames);
    if (nnames == 0) {
        die_at(loc.file, loc.line, loc.col,
               "package '%s' directory '%s' has no .c files", name, dir);
    }
    Package *pkg = xmalloc(sizeof(Package));
    memset(pkg, 0, sizeof(*pkg));
    pkg->name = xstrdup(name);
    pkg->dir = dir;
    pkg->files = xmalloc(nnames * sizeof(TranslationUnit));
    pkg->nfiles = nnames;
    pkg->owns_files = 1;
    pkgs_push(ctx, pkg);
    for (size_t i = 0; i < nnames; i++) {
        char *path = path_join(dir, names[i]);
        char *src = read_file(path);
        TokenArray tokens;
        token_array_init(&tokens);
        lex(src, path, &tokens);
        tu_init(&pkg->files[i]);
        parse_in_pkg(&tokens, &pkg->files[i], ctx);
        if (!pkg->files[i].package.name
            || strcmp(pkg->files[i].package.name, name) != 0) {
            die_at(pkg->files[i].package.loc.file,
                   pkg->files[i].package.loc.line,
                   pkg->files[i].package.loc.col,
                   "package name '%s' does not match directory '%s'",
                   pkg->files[i].package.name
                       ? pkg->files[i].package.name : "(none)",
                   name);
        }
        token_array_free(&tokens);
        free(src);
        (void)path;
        free(names[i]);
    }
    free(names);
    build_exports(pkg);
    loading_pop(ctx);
    return pkg;
}
Package *pkg_register_tus(PkgContext *ctx, const char *name,
                          TranslationUnit **tus, size_t ntus) {
    if (!name || !ntus) return ((void*)0);
    Package *exist = pkg_find(ctx, name);
    if (exist) {
        fprintf(stderr,
                "fakecc: cannot register package '%s': already loaded from '%s'\n",
                name, exist->dir ? exist->dir : "(memory)");
        exit(1);
    }
    Package *pkg = xmalloc(sizeof(Package));
    memset(pkg, 0, sizeof(*pkg));
    pkg->name = xstrdup(name);
    pkg->dir = ((void*)0);
    pkg->owns_files = 0;
    pkg->files = xmalloc(ntus * sizeof(TranslationUnit));
    pkg->nfiles = ntus;
    for (size_t i = 0; i < ntus; i++)
        pkg->files[i] = *tus[i];
    pkgs_push(ctx, pkg);
    build_exports(pkg);
    return pkg;
}
