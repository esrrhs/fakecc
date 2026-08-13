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
void parse(const TokenArray *tokens, TranslationUnit *tu);
extern int isdigit(int c);
extern int isalpha(int c);
extern int isalnum(int c);
extern int isxdigit(int c);
extern int isspace(int c);
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
extern double strtod(const char *s, char **end);
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
struct Parser {
    const TokenArray *tokens;
    size_t pos;
    TranslationUnit *tu;
    StmtArray prepend;
    int anon_counter;
};typedef struct Parser Parser;
static const Token *peek(const Parser *p) {
    return &p->tokens->data[p->pos];
}
static const Token *advance(Parser *p) {
    return &p->tokens->data[p->pos++];
}
static int is_name_token(TokenKind k) {
    return k == TK_IDENT || k == TK_KW_PACKAGE;
}
static void parse_struct_body(Parser *p, StructDef *sd);
static void parse_enum_body(Parser *p, EnumDef *ed);
static int int_literal_value(const char *text);
static Type parse_declarator(Parser *p, Type base, char **name_out);
static int skip_attribute(Parser *p);
static void expect_kind(Parser *p, TokenKind kind, const char *msg) {
    const Token *t = peek(p);
    if (t->kind != kind) {
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected %s but got '%s'", msg, t->text);
    }
    advance(p);
}
static int skip_attribute(Parser *p) {
    if (peek(p)->kind != TK_IDENT
        || strcmp(peek(p)->text, "__attribute__") != 0)
        return 0;
    advance(p);
    if (peek(p)->kind != TK_LPAREN) return 0;
    advance(p);
    if (peek(p)->kind != TK_LPAREN) {
        return 0;
    }
    advance(p);
    int depth = 2;
    while (depth > 0 && peek(p)->kind != TK_EOF) {
        if (peek(p)->kind == TK_LPAREN) depth++;
        else if (peek(p)->kind == TK_RPAREN) depth--;
        advance(p);
    }
    return 1;
}
static int is_type_start(const Parser *p, size_t pos) {
    TokenKind k = p->tokens->data[pos].kind;
    if (k == TK_KW_VOID || k == TK_KW_INT || k == TK_KW_CHAR || k == TK_KW_SHORT
        || k == TK_KW_LONG || k == TK_KW_SIGNED || k == TK_KW_UNSIGNED
        || k == TK_KW_FLOAT || k == TK_KW_DOUBLE || k == TK_KW_BOOL
        || k == TK_KW_STRUCT || k == TK_KW_ENUM || k == TK_KW_UNION
        || k == TK_KW_CONST || k == TK_KW_STATIC || k == TK_KW_EXTERN
        || k == TK_KW_VOLATILE || k == TK_KW_RESTRICT || k == TK_KW_INLINE)
        return 1;
    if (k == TK_IDENT
        && typedef_registry_find(&p->tu->typedefs, p->tokens->data[pos].text))
        return 1;
    return 0;
}
static Type parse_specifiers(Parser *p) {
    int is_const = 0, is_volatile = 0, is_restrict = 0;
    for (;;) {
        if (peek(p)->kind == TK_KW_CONST) { is_const = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_VOLATILE) { is_volatile = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_RESTRICT) { is_restrict = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_INLINE) { advance(p); }
        else break;
    }
    if (peek(p)->kind == TK_KW_VOID) {
        advance(p);
        Type t = type_make_void();
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_KW_STRUCT) {
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag);
            Type t = type_make_struct(tag, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected struct tag but got '%s'", tag->text);
        }
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of struct '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, peek(p)->loc);
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag->text);
            Type t = type_make_struct(tag->text, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        int size = sd ? sd->size : 0;
        Type t = type_make_struct(tag->text, size);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_KW_UNION) {
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            sd->is_union = 1;
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag);
            Type t = type_make_struct(tag, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected union tag but got '%s'", tag->text);
        }
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of union '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, peek(p)->loc);
            sd->is_union = 1;
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag->text);
            Type t = type_make_struct(tag->text, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        int size = sd ? sd->size : 0;
        Type t = type_make_struct(tag->text, size);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_KW_FLOAT) {
        advance(p);
        Type t = type_make_float(4);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_KW_DOUBLE) {
        advance(p);
        Type t = type_make_float(8);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_KW_LONG
        && p->pos + 1 < p->tokens->len
        && p->tokens->data[p->pos + 1].kind == TK_KW_DOUBLE) {
        advance(p);
        advance(p);
        Type t = type_make_float(16);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_KW_ENUM) {
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            EnumDef *ed = enum_registry_add(&p->tu->enums, tag, peek(p)->loc);
            parse_enum_body(p, ed);
            Type t = type_default_int();
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected enum tag but got '%s'", tag->text);
        }
        advance(p);
        Type t = type_default_int();
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_IDENT) {
        const Type *alias = typedef_registry_find(&p->tu->typedefs, peek(p)->text);
        if (alias) {
            advance(p);
            Type t = type_clone(*alias);
            if (t.kind == TY_STRUCT && t.tag) {
                const StructDef *sd = struct_registry_find(&p->tu->structs, t.tag);
                if (sd) t.width = sd->size;
            }
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
    }
    int is_unsigned = 0;
    int saw_sign = 0;
    int width = -1;
    if (peek(p)->kind == TK_KW_BOOL) {
        advance(p);
        Type t = type_make_bool();
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    TokenKind k = peek(p)->kind;
    if (k == TK_KW_SIGNED) { is_unsigned = 0; saw_sign = 1; advance(p); }
    else if (k == TK_KW_UNSIGNED) { is_unsigned = 1; saw_sign = 1; advance(p); }
    k = peek(p)->kind;
    switch (k) {
    case TK_KW_CHAR: advance(p); width = 1; break;
    case TK_KW_SHORT: advance(p); width = 2; break;
    case TK_KW_INT: advance(p); width = 4; break;
    case TK_KW_LONG:
        advance(p); width = 8;
        if (peek(p)->kind == TK_KW_LONG) advance(p);
        if (peek(p)->kind == TK_KW_INT) advance(p);
        break;
    default:
        if (saw_sign) { width = 4; break; }
        {
            const Token *t = peek(p);
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected type but got '%s'", t->text);
        }
    }
    Type t = type_make_int(width, is_unsigned);
    t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
    return t;
}
static Type parse_type(Parser *p, char **name_out);
static void parse_struct_body(Parser *p, StructDef *sd) {
    const char *tag = sd->tag;
    advance(p);
    while (peek(p)->kind != TK_RBRACE) {
        Type base = parse_specifiers(p);
        sd = struct_registry_find(&p->tu->structs, tag);
        if (peek(p)->kind == TK_SEMICOLON) {
            advance(p);
            type_free(&base);
            continue;
        }
        for (;;) {
            char *mname = ((void*)0);
            Type mty = parse_declarator(p, type_clone(base), &mname);
            sd = struct_registry_find(&p->tu->structs, tag);
            if (!mname) {
                const Token *mn = peek(p);
                if (!is_name_token(mn->kind)) {
                    die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                           "expected member name but got '%s'", mn->text);
                }
                mname = xstrdup(mn->text);
                advance(p);
            }
            int bit_width = 0;
            if (peek(p)->kind == TK_COLON) {
                advance(p);
                const Token *w = peek(p);
                if (w->kind != TK_INT_LITERAL) {
                    die_at(w->loc.file, w->loc.line, w->loc.col,
                           "expected bitfield width but got '%s'", w->text);
                }
                bit_width = int_literal_value(w->text);
                advance(p);
            }
            struct_def_push_member(sd, mname, mty, bit_width);
            free(mname);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                continue;
            }
            expect_kind(p, TK_SEMICOLON, "';'");
            break;
        }
        type_free(&base);
    }
    expect_kind(p, TK_RBRACE, "'}'");
    struct_def_finish(sd);
    sd = struct_registry_find(&p->tu->structs, tag);
    if (sd) struct_def_fixup_self_types(sd);
}
static void parse_enum_body(Parser *p, EnumDef *ed) {
    advance(p);
    while (peek(p)->kind != TK_RBRACE) {
        const Token *cn = peek(p);
        if (cn->kind != TK_IDENT) {
            die_at(cn->loc.file, cn->loc.line, cn->loc.col,
                   "expected enum constant name but got '%s'", cn->text);
        }
        advance(p);
        int has_value = 0, value = 0;
        if (peek(p)->kind == TK_ASSIGN) {
            advance(p);
            int sign = 1;
            if (peek(p)->kind == TK_MINUS) { sign = -1; advance(p); }
            else if (peek(p)->kind == TK_PLUS) { advance(p); }
            const Token *v = peek(p);
            if (v->kind == TK_INT_LITERAL) {
                has_value = 1; value = sign * int_literal_value(v->text); advance(p);
            } else if (v->kind == TK_IDENT) {
                const EnumConstant *ec =
                    enum_registry_find_constant(&p->tu->enums, v->text);
                if (!ec) {
                    die_at(v->loc.file, v->loc.line, v->loc.col,
                           "enum value '%s' is not a constant", v->text);
                }
                has_value = 1; value = ec->value; advance(p);
            } else {
                die_at(v->loc.file, v->loc.line, v->loc.col,
                       "expected integer value for enum constant '%s'", cn->text);
            }
        }
        enum_def_push_constant(ed, cn->text, has_value, value, cn->loc);
        if (peek(p)->kind == TK_COMMA) advance(p);
    }
    expect_kind(p, TK_RBRACE, "'}'");
}
static Type make_func_type(Type ret, ParamArray *params) {
    Type **ptys = ((void*)0);
    if (params->len > 0) {
        ptys = malloc(params->len * sizeof(Type *));
        if (!ptys) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        for (size_t i = 0; i < params->len; i++)
            ptys[i] = &params->data[i].type;
    }
    Type t = type_make_func(ret, ptys, (int)params->len);
    free(ptys);
    param_array_free(params);
    return t;
}
static ParamArray parse_param_list(Parser *p) {
    ParamArray params;
    param_array_init(&params);
    if (peek(p)->kind == TK_KW_VOID
        && p->tokens->data[p->pos + 1].kind == TK_RPAREN) {
        advance(p);
        return params;
    }
    if (peek(p)->kind != TK_RPAREN) {
        for (;;) {
            if (!is_type_start(p, p->pos)) {
                const Token *t = peek(p);
                die_at(t->loc.file, t->loc.line, t->loc.col,
                       "expected type for parameter but got '%s'", t->text);
            }
            char *pname_in = ((void*)0);
            Type pty = parse_type(p, &pname_in);
            const Token *pname = peek(p);
            if (!pname_in) {
                pname_in = xstrdup("");
            }
            param_array_push(&params, pname_in, pty, pname->loc);
            free(pname_in);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                if (peek(p)->kind == TK_ELLIPSIS) {
                    advance(p);
                }
                continue;
            }
            break;
        }
        if (params.len > 16) {
            die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                   "more than 16 parameters not supported");
        }
    }
    return params;
}
static int int_literal_value(const char *text);
static int parse_array_size(Parser *p) {
    if (peek(p)->kind == TK_INT_LITERAL) {
        int v = int_literal_value(peek(p)->text);
        advance(p);
        return v;
    }
    if (peek(p)->kind == TK_IDENT) {
        const EnumConstant *ec =
            enum_registry_find_constant(&p->tu->enums, peek(p)->text);
        if (ec) { advance(p); return ec->value; }
    }
    return 0;
}
static void parse_group_content(Parser *p, char **name_out, int *out_ptrs,
                                int *out_array_len, int *out_has_array) {
    int ptrs = 0;
    while (peek(p)->kind == TK_STAR) { advance(p); ptrs++; }
    if (peek(p)->kind == TK_IDENT) {
        *name_out = xstrdup(peek(p)->text);
        advance(p);
    } else {
        *name_out = ((void*)0);
    }
    int array_len = 0;
    int has_array = 0;
    if (peek(p)->kind == TK_LBRACKET) {
        advance(p);
        array_len = parse_array_size(p);
        expect_kind(p, TK_RBRACKET, "']'");
        has_array = 1;
    }
    *out_ptrs = ptrs;
    *out_array_len = array_len;
    *out_has_array = has_array;
}
static Type ptr_wrap(Type t, int is_const, int is_volatile, int is_restrict) {
    Type w = type_make_ptr(t);
    w.is_const = is_const;
    w.is_volatile = is_volatile;
    w.is_restrict = is_restrict;
    return w;
}
static Type parse_declarator(Parser *p, Type base, char **name_out) {
    enum { MAX_PTRS = 8 };
    int ptr_const[MAX_PTRS], ptr_volatile[MAX_PTRS], ptr_restrict[MAX_PTRS];
    int ptrs = 0;
    while (peek(p)->kind == TK_STAR) {
        advance(p);
        int c = 0, v = 0, r = 0;
        while (peek(p)->kind == TK_KW_CONST
               || peek(p)->kind == TK_KW_VOLATILE
               || peek(p)->kind == TK_KW_RESTRICT) {
            if (peek(p)->kind == TK_KW_CONST) c = 1;
            else if (peek(p)->kind == TK_KW_VOLATILE) v = 1;
            else r = 1;
            advance(p);
        }
        if (ptrs >= MAX_PTRS) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                     peek(p)->loc.col, "too many pointer levels");
        ptr_const[ptrs] = c; ptr_volatile[ptrs] = v; ptr_restrict[ptrs] = r;
        ptrs++;
    }
    Type t;
    if (peek(p)->kind == TK_LPAREN) {
        advance(p);
        char *inner_name = ((void*)0);
        int inner_ptrs = 0, inner_array_len = 0, inner_has_array = 0;
        parse_group_content(p, &inner_name, &inner_ptrs, &inner_array_len,
                            &inner_has_array);
        *name_out = inner_name;
        expect_kind(p, TK_RPAREN, "')'");
        t = base;
        int dims[8], ndims = 0;
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                int len = parse_array_size(p);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims++] = len;
            } else {
                advance(p);
                ParamArray params = parse_param_list(p);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(base, &params);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
        for (int i = 0; i < inner_ptrs; i++) t = type_make_ptr(t);
        if (inner_has_array) t = type_make_array(t, inner_array_len);
    } else if (peek(p)->kind == TK_IDENT) {
        *name_out = xstrdup(peek(p)->text);
        advance(p);
        t = base;
        for (int i = 0; i < ptrs; i++)
            t = ptr_wrap(t, ptr_const[i], ptr_volatile[i], ptr_restrict[i]);
        ptrs = 0;
        int dims[8], ndims = 0;
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                int len = parse_array_size(p);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims++] = len;
            } else {
                advance(p);
                ParamArray params = parse_param_list(p);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(base, &params);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
    } else {
        *name_out = ((void*)0);
        t = base;
        int dims[8], ndims = 0;
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                int len = parse_array_size(p);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims++] = len;
            } else {
                advance(p);
                ParamArray params = parse_param_list(p);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(base, &params);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
    }
    for (int i = 0; i < ptrs; i++)
        t = ptr_wrap(t, ptr_const[i], ptr_volatile[i], ptr_restrict[i]);
    return t;
}
static Type parse_type(Parser *p, char **name_out) {
    Type base = parse_specifiers(p);
    return parse_declarator(p, base, name_out);
}
static Type parse_type_abstract(Parser *p) {
    Type base = parse_specifiers(p);
    Type t = base;
    while (peek(p)->kind == TK_STAR) {
        advance(p);
        Type w = type_make_ptr(t);
        type_free(&t);
        t = w;
    }
    return t;
}
static Type parse_type_name(Parser *p) {
    Type base = parse_specifiers(p);
    int ptrs = 0;
    while (peek(p)->kind == TK_STAR) { advance(p); ptrs++; }
    int dims[8], ndims = 0;
    while (peek(p)->kind == TK_LBRACKET) {
        advance(p);
        int len = parse_array_size(p);
        expect_kind(p, TK_RBRACKET, "']'");
        if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                   peek(p)->loc.col, "too many array dimensions");
        dims[ndims++] = len;
    }
    if (ptrs == 0 && ndims == 0)
        return base;
    Type t = base;
    for (int i = ndims - 1; i >= 0; i--) {
        Type w = type_make_array(t, dims[i]);
        type_free(&t);
        t = w;
    }
    for (int i = 0; i < ptrs; i++) {
        Type w = type_make_ptr(t);
        type_free(&t);
        t = w;
    }
    return t;
}
static Expr *parse_expr(Parser *p);
static Expr *parse_comma(Parser *p);
static Expr *parse_assign(Parser *p);
static Expr *parse_ternary(Parser *p);
static Expr *parse_or(Parser *p);
static Expr *parse_and(Parser *p);
static Expr *parse_bitor(Parser *p);
static Expr *parse_xor(Parser *p);
static Expr *parse_bitand(Parser *p);
static Expr *parse_equality(Parser *p);
static Expr *parse_relational(Parser *p);
static Expr *parse_shift(Parser *p);
static Expr *parse_add(Parser *p);
static Expr *parse_mul(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_postfix(Parser *p, Expr *lhs);
static Expr *parse_primary(Parser *p);
static Expr *parse_init_list(Parser *p);
static Expr *parse_expr(Parser *p) {
    return parse_comma(p);
}
static Expr *parse_comma(Parser *p) {
    Expr *lhs = parse_assign(p);
    while (peek(p)->kind == TK_COMMA) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_assign(p);
        lhs = expr_new_comma(lhs, rhs, loc);
    }
    return lhs;
}
static int compound_op(TokenKind k, BinOp *op) {
    switch (k) {
    case TK_PLUS_EQ: *op = BOP_ADD; break;
    case TK_MINUS_EQ: *op = BOP_SUB; break;
    case TK_STAR_EQ: *op = BOP_MUL; break;
    case TK_SLASH_EQ: *op = BOP_DIV; break;
    case TK_PERCENT_EQ: *op = BOP_MOD; break;
    case TK_AMP_EQ: *op = BOP_BITAND; break;
    case TK_BITOR_EQ: *op = BOP_BITOR; break;
    case TK_XOR_EQ: *op = BOP_BITXOR; break;
    case TK_SHL_EQ: *op = BOP_SHL; break;
    case TK_SHR_EQ: *op = BOP_SHR; break;
    default: return 0;
    }
    return 1;
}
static Expr *parse_assign(Parser *p) {
    Expr *lhs = parse_ternary(p);
    TokenKind k = peek(p)->kind;
    if (k == TK_ASSIGN) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_assign(p);
        return expr_new_assign(lhs, rhs, loc);
    }
    BinOp op;
    if (compound_op(k, &op)) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_assign(p);
        return expr_new_compound_assign(lhs, rhs, op, loc);
    }
    return lhs;
}
static Expr *parse_ternary(Parser *p) {
    Expr *cond = parse_or(p);
    if (peek(p)->kind != TK_QUESTION) return cond;
    SourceLoc loc = peek(p)->loc;
    advance(p);
    Expr *then = parse_expr(p);
    expect_kind(p, TK_COLON, "':'");
    Expr *else_ = parse_ternary(p);
    return expr_new_ternary(cond, then, else_, loc);
}
static Expr *parse_or(Parser *p) {
    Expr *lhs = parse_and(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_OROR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_and(p);
        lhs = expr_new_binop(BOP_OR, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_and(Parser *p) {
    Expr *lhs = parse_bitor(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_ANDAND) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_bitor(p);
        lhs = expr_new_binop(BOP_AND, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_bitor(Parser *p) {
    Expr *lhs = parse_xor(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_BITOR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_xor(p);
        lhs = expr_new_binop(BOP_BITOR, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_xor(Parser *p) {
    Expr *lhs = parse_bitand(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_XOR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_bitand(p);
        lhs = expr_new_binop(BOP_BITXOR, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_bitand(Parser *p) {
    Expr *lhs = parse_equality(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_AMP) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_equality(p);
        lhs = expr_new_binop(BOP_BITAND, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_equality(Parser *p) {
    Expr *lhs = parse_relational(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_EQ && k != TK_NE) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_relational(p);
        BinOp op = (k == TK_EQ) ? BOP_EQ : BOP_NE;
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_relational(Parser *p) {
    Expr *lhs = parse_shift(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_LT && k != TK_LE && k != TK_GT && k != TK_GE) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_shift(p);
        BinOp op;
        switch (k) {
        case TK_LT: op = BOP_LT; break;
        case TK_LE: op = BOP_LE; break;
        case TK_GT: op = BOP_GT; break;
        default: op = BOP_GE; break;
        }
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_shift(Parser *p) {
    Expr *lhs = parse_add(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_SHL && k != TK_SHR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_add(p);
        BinOp op = (k == TK_SHL) ? BOP_SHL : BOP_SHR;
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_add(Parser *p) {
    Expr *lhs = parse_mul(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_PLUS && k != TK_MINUS) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_mul(p);
        BinOp op = (k == TK_PLUS) ? BOP_ADD : BOP_SUB;
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_mul(Parser *p) {
    Expr *lhs = parse_unary(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_STAR && k != TK_SLASH && k != TK_PERCENT) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_unary(p);
        BinOp op;
        switch (k) {
        case TK_STAR: op = BOP_MUL; break;
        case TK_SLASH: op = BOP_DIV; break;
        default: op = BOP_MOD; break;
        }
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}
static Expr *parse_unary(Parser *p) {
    TokenKind k = peek(p)->kind;
    if (k == TK_PLUS || k == TK_MINUS) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *operand = parse_unary(p);
        UnaryOp op = (k == TK_MINUS) ? UOP_NEG : UOP_POS;
        return expr_new_unary(op, operand, loc);
    }
    if (k == TK_TILDE) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_unary(UOP_BITNOT, parse_unary(p), loc);
    }
    if (k == TK_NOT) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_unary(UOP_NOT, parse_unary(p), loc);
    }
    if (k == TK_INC || k == TK_DEC) {
        SourceLoc loc = peek(p)->loc;
        int is_inc = (k == TK_INC);
        advance(p);
        return expr_new_inc_dec(parse_unary(p), is_inc, 1 , loc);
    }
    if (k == TK_AMP) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_addr(parse_unary(p), loc);
    }
    if (k == TK_STAR) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_deref(parse_unary(p), loc);
    }
    if (k == TK_KW_SIZEOF) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        if (peek(p)->kind == TK_LPAREN && is_type_start(p, p->pos + 1)) {
            advance(p);
            Type t = parse_type_abstract(p);
            expect_kind(p, TK_RPAREN, "')'");
            Expr *e = expr_new_sizeof_type(t, loc);
            type_free(&t);
            return e;
        }
        return expr_new_sizeof_expr(parse_unary(p), loc);
    }
    if (k == TK_KW_ALIGNOF) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        if (peek(p)->kind != TK_LPAREN)
            die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                   "expected '(' after '_Alignof'");
        advance(p);
        Type t = parse_type_abstract(p);
        expect_kind(p, TK_RPAREN, "')'");
        Expr *e = expr_new_alignof_type(t, loc);
        type_free(&t);
        return e;
    }
    return parse_primary(p);
}
static Expr *parse_postfix(Parser *p, Expr *lhs) {
    for (;;) {
        if (peek(p)->kind == TK_LBRACKET) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            Expr *idx = parse_expr(p);
            expect_kind(p, TK_RBRACKET, "']'");
            lhs = expr_new_index(lhs, idx, loc);
            continue;
        }
        if (peek(p)->kind == TK_DOT) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            const Token *mn = peek(p);
            if (!is_name_token(mn->kind)) {
                die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                       "expected member name after '.'");
            }
            advance(p);
            lhs = expr_new_member(lhs, mn->text, loc);
            continue;
        }
        if (peek(p)->kind == TK_ARROW) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            const Token *mn = peek(p);
            if (!is_name_token(mn->kind)) {
                die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                       "expected member name after '->'");
            }
            advance(p);
            Expr *deref = expr_new_deref(lhs, loc);
            lhs = expr_new_member(deref, mn->text, loc);
            continue;
        }
        if (peek(p)->kind == TK_INC || peek(p)->kind == TK_DEC) {
            SourceLoc loc = peek(p)->loc;
            int is_inc = (peek(p)->kind == TK_INC);
            advance(p);
            lhs = expr_new_inc_dec(lhs, is_inc, 0 , loc);
            continue;
        }
        if (peek(p)->kind == TK_LPAREN) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            Expr *call = expr_new_call(lhs, loc);
            int is_va_arg = (lhs->kind == EX_VAR
                             && strcmp(lhs->u.var.name, "va_arg") == 0);
            if (peek(p)->kind != TK_RPAREN) {
                for (;;) {
                    Expr *arg = parse_assign(p);
                    expr_call_push_arg(call, arg);
                    if (peek(p)->kind == TK_COMMA) {
                        advance(p);
                        if (is_va_arg && call->u.call.args.len == 1) {
                            Type t = parse_type_abstract(p);
                            call->va_arg_type = t;
                            break;
                        }
                        continue;
                    }
                    break;
                }
            }
            expect_kind(p, TK_RPAREN, "')'");
            lhs = call;
            continue;
        }
        break;
    }
    return lhs;
}
static void float_literal_width(const char *text, int *out_width) {
    size_t len = strlen(text);
    *out_width = 8;
    if (len > 0) {
        char last = text[len - 1];
        if (last == 'f' || last == 'F')
            *out_width = 4;
        else if (last == 'l' || last == 'L')
            *out_width = 16;
    }
}
static int int_literal_value(const char *text) {
    return (int)strtol(text, ((void*)0), 0);
}
static int char_literal_value(const char *text) {
    if (text[1] == '\\') {
        if (text[2] == 'x' || text[2] == 'X') {
            int val = 0;
            int i = 3;
            while (text[i] != '\0' && isxdigit((unsigned char)text[i])) {
                char c = text[i];
                int d = (c >= '0' && c <= '9') ? c - '0'
                      : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                      : c - 'A' + 10;
                val = val * 16 + d;
                i++;
            }
            return val & 0xff;
        }
        if (text[2] >= '0' && text[2] <= '7') {
            int val = 0;
            int i = 2;
            int n = 0;
            while (text[i] != '\0' && n < 3 && text[i] >= '0' && text[i] <= '7') {
                val = val * 8 + (text[i] - '0');
                i++; n++;
            }
            return val & 0xff;
        }
        switch (text[2]) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '\\': return '\\';
        case '\'': return '\'';
        case '0': return '\0';
        default: return text[2];
        }
    }
    return (unsigned char)text[1];
}
static Expr *parse_primary(Parser *p) {
    const Token *t = peek(p);
    if (t->kind == TK_INT_LITERAL) {
        Expr *e = expr_new_int(int_literal_value(t->text), t->loc);
        advance(p);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_FLOAT_LITERAL) {
        int width = 8;
        float_literal_width(t->text, &width);
        Expr *e = expr_new_float_lit(t->text, width, t->loc);
        advance(p);
        return e;
    }
    if (t->kind == TK_STRING_LITERAL) {
        int total = 0;
        char *buf = ((void*)0);
        while (peek(p)->kind == TK_STRING_LITERAL) {
            const char *src = peek(p)->text;
            size_t slen = strlen(src);
            if (slen >= 1 && src[0] == '"') { src++; slen--; }
            if (slen >= 1 && src[slen-1] == '"') slen--;
            char *seg = malloc(slen + 1);
            int slen2 = 0;
            for (size_t i = 0; i < slen; i++) {
                char c = src[i];
                if (c == '\\' && i + 1 < slen) {
                    i++;
                    if (src[i] == 'x' || src[i] == 'X') {
                        int val = 0;
                        while (i + 1 < slen && isxdigit((unsigned char)src[i + 1])) {
                            i++;
                            char h = src[i];
                            int d = (h >= '0' && h <= '9') ? h - '0'
                                  : (h >= 'a' && h <= 'f') ? h - 'a' + 10
                                  : h - 'A' + 10;
                            val = val * 16 + d;
                        }
                        seg[slen2++] = (char)(val & 0xff);
                    } else if (src[i] >= '0' && src[i] <= '7') {
                        int val = src[i] - '0';
                        int n = 1;
                        while (n < 3 && i + 1 < slen
                               && src[i + 1] >= '0' && src[i + 1] <= '7') {
                            i++;
                            val = val * 8 + (src[i] - '0');
                            n++;
                        }
                        seg[slen2++] = (char)(val & 0xff);
                    } else switch (src[i]) {
                    case 'n': seg[slen2++] = '\n'; break;
                    case 't': seg[slen2++] = '\t'; break;
                    case 'r': seg[slen2++] = '\r'; break;
                    case '0': seg[slen2++] = '\0'; break;
                    case '\\': seg[slen2++] = '\\'; break;
                    case '"': seg[slen2++] = '"'; break;
                    case '\'': seg[slen2++] = '\''; break;
                    default: seg[slen2++] = src[i]; break;
                    }
                } else {
                    seg[slen2++] = c;
                }
            }
            buf = realloc(buf, (size_t)total + slen2 + 1);
            memcpy(buf + total, seg, slen2);
            total += slen2;
            free(seg);
            advance(p);
        }
        Expr *e = expr_new_str(buf, total, t->loc);
        free(buf);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_CHAR_LITERAL) {
        Expr *e = expr_new_int(char_literal_value(t->text), t->loc);
        advance(p);
        return e;
    }
    if (t->kind == TK_IDENT) {
        const Token *ident = t;
        advance(p);
        {
            const EnumConstant *ec =
                enum_registry_find_constant(&p->tu->enums, ident->text);
            if (ec)
                return expr_new_int(ec->value, ident->loc);
        }
        return parse_postfix(p, expr_new_var(ident->text, ident->loc));
    }
    if (t->kind == TK_LPAREN) {
        if (is_type_start(p, p->pos + 1)) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            Type ty = parse_type_name(p);
            expect_kind(p, TK_RPAREN, "')'");
            if (peek(p)->kind == TK_LBRACE) {
                Expr *init = parse_init_list(p);
                Expr *e = expr_new_compound_literal(ty, init, loc);
                type_free(&ty);
                return parse_postfix(p, e);
            }
            Expr *e = expr_new_cast(ty, parse_unary(p), loc);
            type_free(&ty);
            return parse_postfix(p, e);
        }
        advance(p);
        Expr *e = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        return parse_postfix(p, e);
    }
    die_at(t->loc.file, t->loc.line, t->loc.col,
           "expected expression but got '%s'", t->text);
    return ((void*)0);
}
static Expr *parse_init_list(Parser *p) {
    SourceLoc loc = peek(p)->loc;
    expect_kind(p, TK_LBRACE, "'{'");
    Expr **elements = ((void*)0);
    int *dkind = ((void*)0), *dindex = ((void*)0);
    char **dmember = ((void*)0);
    int num = 0, cap = 0;
    while (peek(p)->kind != TK_RBRACE) {
        if (peek(p)->kind == TK_EOF) {
            die_at(loc.file, loc.line, loc.col,
                   "unterminated initializer list");
        }
        int kind = -1, idx = -1;
        char *member = ((void*)0);
        if (peek(p)->kind == TK_DOT) {
            advance(p);
            const Token *fn = peek(p);
            if (fn->kind != TK_IDENT)
                die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                       "expected field name after '.' but got '%s'", fn->text);
            advance(p);
            member = xstrdup(fn->text);
            expect_kind(p, TK_ASSIGN, "'='");
            kind = 1;
        } else if (peek(p)->kind == TK_LBRACKET) {
            advance(p);
            const Token *ix = peek(p);
            if (ix->kind != TK_INT_LITERAL)
                die_at(ix->loc.file, ix->loc.line, ix->loc.col,
                       "expected integer constant in designator but got '%s'",
                       ix->text);
            idx = int_literal_value(ix->text);
            advance(p);
            expect_kind(p, TK_RBRACKET, "']'");
            expect_kind(p, TK_ASSIGN, "'='");
            kind = 0;
        }
        Expr *elem;
        if (peek(p)->kind == TK_LBRACE) {
            elem = parse_init_list(p);
        } else {
            elem = parse_assign(p);
        }
        if (num >= cap) {
            cap = cap ? cap * 2 : 8;
            elements = realloc(elements, cap * sizeof(Expr *));
            dkind = realloc(dkind, cap * sizeof(int));
            dindex = realloc(dindex, cap * sizeof(int));
            dmember = realloc(dmember, cap * sizeof(char *));
        }
        elements[num] = elem;
        dkind[num] = kind;
        dindex[num] = idx;
        dmember[num] = member;
        num++;
        if (peek(p)->kind == TK_COMMA) {
            advance(p);
            if (peek(p)->kind == TK_RBRACE) break;
        } else if (peek(p)->kind != TK_RBRACE) {
            const Token *t = peek(p);
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected ',' or '}' in initializer list but got '%s'",
                   t->text);
        }
    }
    expect_kind(p, TK_RBRACE, "'}'");
    Expr *list = expr_new_init_list(elements, num, loc);
    for (int i = 0; i < num; i++) {
        list->u.init_list.desig_kind[i] = dkind[i];
        list->u.init_list.desig_index[i] = dindex[i];
        list->u.init_list.desig_member[i] = dmember[i];
    }
    free(dkind);
    free(dindex);
    free(dmember);
    return list;
}
static void parse_stmt_list(Parser *p, StmtArray *out);
static Stmt parse_stmt(Parser *p);
static Stmt parse_typedef_stmt(Parser *p);
static Stmt parse_switch(Parser *p);
static void parse_stmt_list(Parser *p, StmtArray *out) {
    while (peek(p)->kind != TK_RBRACE) {
        const Token *t = peek(p);
        if (t->kind == TK_EOF) {
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected '}' but got end of file");
        }
        for (size_t i = 0; i < p->prepend.len; i++)
            stmt_array_push(out, p->prepend.data[i]);
        p->prepend.len = 0;
        stmt_array_push(out, parse_stmt(p));
    }
    for (size_t i = 0; i < p->prepend.len; i++)
        stmt_array_push(out, p->prepend.data[i]);
    p->prepend.len = 0;
}
static Stmt parse_stmt(Parser *p) {
    TokenKind k = peek(p)->kind;
    if (k == TK_KW_TYPEDEF || is_type_start(p, p->pos)) {
        if (k == TK_KW_TYPEDEF) {
            return parse_typedef_stmt(p);
        }
        SourceLoc decl_loc = peek(p)->loc;
        int storage_class = 0;
        if (peek(p)->kind == TK_KW_STATIC) {
            storage_class = 1;
            advance(p);
        } else if (peek(p)->kind == TK_KW_EXTERN) {
            storage_class = 2;
            advance(p);
        }
        Type base = parse_specifiers(p);
        if (peek(p)->kind == TK_SEMICOLON) {
            advance(p);
            Stmt s;
            memset(&s, 0, sizeof(s));
            s.kind = ST_BLOCK;
            s.loc = decl_loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        StmtArray decls;
        stmt_array_init(&decls);
        for (;;) {
            char *decl_name = ((void*)0);
            Type ty = parse_declarator(p, type_clone(base), &decl_name);
            if (!decl_name) {
                const Token *name = peek(p);
                if (name->kind != TK_IDENT) {
                    die_at(name->loc.file, name->loc.line, name->loc.col,
                           "expected variable name but got '%s'", name->text);
                }
                decl_name = xstrdup(name->text);
                advance(p);
            }
            Stmt s;
            s.kind = ST_DECL;
            s.loc = decl_loc;
            s.u.decl.name = decl_name;
            s.u.decl.type = ty;
            s.u.decl.storage_class = storage_class;
            s.u.decl.init = ((void*)0);
            if (peek(p)->kind == TK_ASSIGN) {
                advance(p);
                if (storage_class == 2) {
                    die_at(s.loc.file, s.loc.line, s.loc.col,
                           "cannot initialize an 'extern' variable");
                }
                if (peek(p)->kind == TK_LBRACE) {
                    s.u.decl.init = parse_init_list(p);
                } else {
                    s.u.decl.init = parse_assign(p);
                }
            }
            stmt_array_push(&decls, s);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                continue;
            }
            break;
        }
        expect_kind(p, TK_SEMICOLON, "';'");
        type_free(&base);
        Stmt first = decls.data[0];
        for (size_t i = 1; i < decls.len; i++)
            stmt_array_push(&p->prepend, decls.data[i]);
        free(decls.data);
        return first;
    }
    if (k == TK_KW_RETURN) {
        const Token *kw = peek(p);
        advance(p);
        Stmt s;
        s.kind = ST_RETURN;
        s.loc = kw->loc;
        if (peek(p)->kind == TK_SEMICOLON) {
            s.u.value = ((void*)0);
        } else {
            s.u.value = parse_expr(p);
        }
        expect_kind(p, TK_SEMICOLON, "';'");
        return s;
    }
    if (k == TK_KW_IF) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Expr *cond = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        Stmt then_s = parse_stmt(p);
        Stmt *then_ptr = stmt_alloc();
        *then_ptr = then_s;
        Stmt *else_ptr = ((void*)0);
        if (peek(p)->kind == TK_KW_ELSE) {
            advance(p);
            Stmt else_s = parse_stmt(p);
            else_ptr = stmt_alloc();
            *else_ptr = else_s;
        }
        Stmt s;
        s.kind = ST_IF;
        s.loc = kw->loc;
        s.u.if_s.cond = cond;
        s.u.if_s.then_s = then_ptr;
        s.u.if_s.else_s = else_ptr;
        return s;
    }
    if (k == TK_KW_WHILE) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Expr *cond = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        Stmt body = parse_stmt(p);
        Stmt *body_ptr = stmt_alloc();
        *body_ptr = body;
        Stmt s;
        s.kind = ST_WHILE;
        s.loc = kw->loc;
        s.u.while_s.cond = cond;
        s.u.while_s.body = body_ptr;
        return s;
    }
    if (k == TK_KW_DO) {
        const Token *kw = peek(p);
        advance(p);
        Stmt body = parse_stmt(p);
        expect_kind(p, TK_KW_WHILE, "'while'");
        expect_kind(p, TK_LPAREN, "'('");
        Expr *cond = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s;
        s.kind = ST_DO_WHILE;
        s.loc = kw->loc;
        s.u.do_s.body = stmt_alloc();
        *s.u.do_s.body = body;
        s.u.do_s.cond = cond;
        return s;
    }
    if (k == TK_KW_FOR) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Stmt *init_ptr = ((void*)0);
        if (peek(p)->kind != TK_SEMICOLON) {
            Stmt is = parse_stmt(p);
            if (p->prepend.len > 0) {
                Stmt blk;
                blk.kind = ST_BLOCK;
                blk.loc = is.loc;
                stmt_array_init(&blk.u.block);
                stmt_array_push(&blk.u.block, is);
                for (size_t pi = 0; pi < p->prepend.len; pi++)
                    stmt_array_push(&blk.u.block, p->prepend.data[pi]);
                p->prepend.len = 0;
                init_ptr = stmt_alloc();
                *init_ptr = blk;
            } else {
                init_ptr = stmt_alloc();
                *init_ptr = is;
            }
        } else {
            advance(p);
        }
        Expr *cond = ((void*)0);
        if (peek(p)->kind != TK_SEMICOLON) cond = parse_expr(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Expr *step = ((void*)0);
        if (peek(p)->kind != TK_RPAREN) step = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        Stmt body = parse_stmt(p);
        Stmt *body_ptr = stmt_alloc();
        *body_ptr = body;
        Stmt s;
        s.kind = ST_FOR;
        s.loc = kw->loc;
        s.u.for_s.init = init_ptr;
        s.u.for_s.cond = cond;
        s.u.for_s.step = step;
        s.u.for_s.body = body_ptr;
        return s;
    }
    if (k == TK_KW_BREAK) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_BREAK; s.loc = kw->loc; return s;
    }
    if (k == TK_KW_CONTINUE) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_CONTINUE; s.loc = kw->loc; return s;
    }
    if (k == TK_KW_GOTO) {
        const Token *kw = peek(p);
        advance(p);
        const Token *label = peek(p);
        if (label->kind != TK_IDENT) {
            die_at(label->loc.file, label->loc.line, label->loc.col,
                   "expected label name after 'goto' but got '%s'", label->text);
        }
        advance(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s;
        s.kind = ST_GOTO;
        s.loc = kw->loc;
        s.u.goto_s.target = xstrdup(label->text);
        return s;
    }
    if (k == TK_LBRACE) {
        const Token *lb = peek(p);
        advance(p);
        Stmt s;
        s.kind = ST_BLOCK;
        s.loc = lb->loc;
        stmt_array_init(&s.u.block);
        parse_stmt_list(p, &s.u.block);
        expect_kind(p, TK_RBRACE, "'}'");
        return s;
    }
    if (k == TK_IDENT && p->tokens->data[p->pos + 1].kind == TK_COLON) {
        const Token *name = peek(p);
        advance(p);
        advance(p);
        Stmt inner = parse_stmt(p);
        Stmt s;
        s.kind = ST_LABEL;
        s.loc = name->loc;
        s.u.label_s.name = xstrdup(name->text);
        s.u.label_s.stmt = stmt_alloc();
        *s.u.label_s.stmt = inner;
        return s;
    }
    if (k == TK_KW_SWITCH) {
        return parse_switch(p);
    }
    const Token *t = peek(p);
    Stmt s;
    s.kind = ST_EXPR;
    s.loc = t->loc;
    s.u.expr = parse_expr(p);
    expect_kind(p, TK_SEMICOLON, "';'");
    return s;
}
static Stmt parse_typedef_stmt(Parser *p) {
    const Token *kw = peek(p);
    advance(p);
    if (peek(p)->kind == TK_KW_STRUCT || peek(p)->kind == TK_KW_UNION) {
        TokenKind base = peek(p)->kind;
        advance(p);
        if (peek(p)->kind == TK_IDENT
            && p->tokens->data[p->pos + 1].kind == TK_LBRACE) {
            const Token *tag = peek(p);
            advance(p);
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, tag->loc);
            sd->is_union = (base == TK_KW_UNION);
            parse_struct_body(p, sd);
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            Type ty = type_make_struct(tag->text, sd->size);
            typedef_registry_add(&p->tu->typedefs, name->text, ty);
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            sd->is_union = (base == TK_KW_UNION);
            parse_struct_body(p, sd);
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            Type ty = type_make_struct(tag, sd->size);
            typedef_registry_add(&p->tu->typedefs, name->text, ty);
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected struct tag but got '%s'", tag->text);
        }
        advance(p);
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        int size = sd ? sd->size : 0;
        Type ty = type_make_struct(tag->text, size);
        char *decl_name = ((void*)0);
        ty = parse_declarator(p, ty, &decl_name);
        const Token *name = peek(p);
        if (!decl_name) {
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            decl_name = xstrdup(name->text);
            advance(p);
        } else if (name->kind == TK_IDENT) {
            advance(p);
        }
        if (typedef_registry_find(&p->tu->typedefs, decl_name)) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "redefinition of typedef '%s'", decl_name);
        }
        typedef_registry_add(&p->tu->typedefs, decl_name, ty);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
        stmt_array_init(&s.u.block);
        return s;
    }
    if (peek(p)->kind == TK_KW_ENUM) {
        advance(p);
        if (peek(p)->kind == TK_IDENT
            && p->tokens->data[p->pos + 1].kind == TK_LBRACE) {
            const Token *tag = peek(p);
            advance(p);
            if (enum_registry_find(&p->tu->enums, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of enum '%s'", tag->text);
            }
            EnumDef *ed = enum_registry_add(&p->tu->enums, tag->text, tag->loc);
            parse_enum_body(p, ed);
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            typedef_registry_add(&p->tu->typedefs, name->text, type_default_int());
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            EnumDef *ed = enum_registry_add(&p->tu->enums, tag, peek(p)->loc);
            parse_enum_body(p, ed);
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            typedef_registry_add(&p->tu->typedefs, name->text, type_default_int());
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected enum tag but got '%s'", tag->text);
        }
        advance(p);
        char *decl_name = ((void*)0);
        Type ty = type_default_int();
        ty = parse_declarator(p, ty, &decl_name);
        const Token *name = peek(p);
        if (!decl_name) {
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            decl_name = xstrdup(name->text);
            advance(p);
        } else if (name->kind == TK_IDENT) {
            advance(p);
        }
        if (typedef_registry_find(&p->tu->typedefs, decl_name)) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "redefinition of typedef '%s'", decl_name);
        }
        typedef_registry_add(&p->tu->typedefs, decl_name, ty);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
        stmt_array_init(&s.u.block);
        return s;
    }
    char *decl_name = ((void*)0);
    Type ty = parse_type(p, &decl_name);
    const Token *name = peek(p);
    if (!decl_name) {
        if (name->kind != TK_IDENT) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "expected typedef name but got '%s'", name->text);
        }
        decl_name = xstrdup(name->text);
        advance(p);
    } else {
        if (name->kind == TK_IDENT) advance(p);
    }
    if (typedef_registry_find(&p->tu->typedefs, decl_name)) {
        die_at(name->loc.file, name->loc.line, name->loc.col,
               "redefinition of typedef '%s'", decl_name);
    }
    typedef_registry_add(&p->tu->typedefs, decl_name, ty);
    expect_kind(p, TK_SEMICOLON, "';'");
    Stmt s;
    s.kind = ST_BLOCK;
    s.loc = kw->loc;
    stmt_array_init(&s.u.block);
    return s;
}
static int case_constant_value(Parser *p, const char *text) {
    if (text[0] >= '0' && text[0] <= '9') {
        return int_literal_value(text);
    }
    const EnumConstant *ec =
        enum_registry_find_constant(&p->tu->enums, text);
    if (ec) return ec->value;
    {
        const Token *t = peek(p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "case label '%s' is not a constant", text);
    }
    return 0;
}
static Stmt parse_switch(Parser *p) {
    const Token *kw = peek(p);
    advance(p);
    expect_kind(p, TK_LPAREN, "'('");
    Expr *cond = parse_expr(p);
    expect_kind(p, TK_RPAREN, "')'");
    expect_kind(p, TK_LBRACE, "'{'");
    Stmt s;
    s.kind = ST_SWITCH;
    s.loc = kw->loc;
    s.u.switch_s.cond = cond;
    s.u.switch_s.cases = ((void*)0);
    s.u.switch_s.num_cases = 0;
    s.u.switch_s.cap_cases = 0;
    while (peek(p)->kind != TK_RBRACE) {
        TokenKind k = peek(p)->kind;
        if (k == TK_KW_CASE) {
            advance(p);
            const Token *cv = peek(p);
            int value;
            if (cv->kind == TK_INT_LITERAL) {
                value = int_literal_value(cv->text);
                advance(p);
            } else if (cv->kind == TK_IDENT) {
                value = case_constant_value(p, cv->text);
                advance(p);
            } else if (cv->kind == TK_CHAR_LITERAL) {
                value = char_literal_value(cv->text);
                advance(p);
            } else {
                die_at(cv->loc.file, cv->loc.line, cv->loc.col,
                       "expected constant case label but got '%s'", cv->text);
            }
            expect_kind(p, TK_COLON, "':'");
            switch_push_case(&s, 0, value);
        } else if (k == TK_KW_DEFAULT) {
            advance(p);
            expect_kind(p, TK_COLON, "':'");
            switch_push_case(&s, 1, 0);
        } else {
            if (s.u.switch_s.num_cases == 0) {
                die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                       "statement before any case label in switch");
            }
            Stmt stmt = parse_stmt(p);
            SwitchCase *arm = &s.u.switch_s.cases[s.u.switch_s.num_cases - 1];
            stmt_array_push(&arm->stmts, stmt);
        }
    }
    expect_kind(p, TK_RBRACE, "'}'");
    return s;
}
static FunctionDecl parse_function_decl(Parser *p) {
    SourceLoc fn_loc = peek(p)->loc;
    int is_extern = 0;
    int is_static = 0;
    for (;;) {
        if (peek(p)->kind == TK_KW_STATIC) { advance(p); is_static = 1; }
        else if (peek(p)->kind == TK_KW_EXTERN) { advance(p); is_extern = 1; }
        else if (peek(p)->kind == TK_KW_INLINE) { advance(p); }
        else break;
    }
    Type ret_ty = parse_type_abstract(p);
    const Token *name = peek(p);
    if (name->kind != TK_IDENT) {
        die_at(name->loc.file, name->loc.line, name->loc.col,
               "expected function name but got '%s'", name->text);
    }
    advance(p);
    expect_kind(p, TK_LPAREN, "'('");
    FunctionDecl fn;
    fn.name = xstrdup(name->text);
    fn.ret_type = ret_ty;
    param_array_init(&fn.params);
    stmt_array_init(&fn.body);
    fn.loc = fn_loc;
    fn.is_variadic = 0;
    fn.is_extern = is_extern;
    fn.is_static = is_static;
    if (peek(p)->kind == TK_KW_VOID
        && p->tokens->data[p->pos + 1].kind == TK_RPAREN) {
        advance(p);
    } else if (peek(p)->kind != TK_RPAREN) {
        for (;;) {
            if (!is_type_start(p, p->pos)) {
                const Token *t = peek(p);
                die_at(t->loc.file, t->loc.line, t->loc.col,
                       "expected type for parameter but got '%s'", t->text);
            }
            char *pname = ((void*)0);
            Type pty = parse_type(p, &pname);
            if (!pname) {
                pname = xstrdup("");
            }
            if (pty.kind == TY_ARRAY) {
                Type elem = *pty.elem_type;
                type_free(&pty);
                pty = type_make_ptr(elem);
                type_free(&elem);
            }
            param_array_push(&fn.params, pname, pty, peek(p)->loc);
            free(pname);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                if (peek(p)->kind == TK_ELLIPSIS) {
                    advance(p);
                    fn.is_variadic = 1;
                    break;
                }
                continue;
            }
            break;
        }
        if (fn.params.len > 16) {
            die_at(fn.loc.file, fn.loc.line, fn.loc.col,
                   "more than 16 parameters not supported");
        }
    }
    if (fn.is_variadic && fn.params.len == 0) {
        die_at(fn.loc.file, fn.loc.line, fn.loc.col,
               "'...' must follow a named parameter");
    }
    expect_kind(p, TK_RPAREN, "')'");
    for (;;) {
        if (!skip_attribute(p)) break;
    }
    if (fn.is_extern || peek(p)->kind == TK_SEMICOLON) {
        expect_kind(p, TK_SEMICOLON, "';'");
        fn.is_extern = 1;
        return fn;
    }
    expect_kind(p, TK_LBRACE, "'{'");
    parse_stmt_list(p, &fn.body);
    expect_kind(p, TK_RBRACE, "'}'");
    return fn;
}
static PackageDecl parse_package_decl(Parser *p) {
    const Token *kw = peek(p);
    expect_kind(p, TK_KW_PACKAGE, "'package'");
    const Token *ident = peek(p);
    if (ident->kind != TK_IDENT) {
        die_at(ident->loc.file, ident->loc.line, ident->loc.col,
               "expected package name but got '%s'", ident->text);
    }
    advance(p);
    expect_kind(p, TK_SEMICOLON, "';'");
    PackageDecl pd;
    pd.name = xstrdup(ident->text);
    pd.loc = kw->loc;
    return pd;
}
void parse(const TokenArray *tokens, TranslationUnit *tu) {
    Parser p;
    p.tokens = tokens;
    p.pos = 0;
    p.tu = tu;
    stmt_array_init(&p.prepend);
    p.anon_counter = 0;
    if (peek(&p)->kind != TK_KW_PACKAGE) {
        const Token *t = peek(&p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected 'package' declaration at start of file");
    }
    tu->package = parse_package_decl(&p);
    if (peek(&p)->kind == TK_KW_IMPORT) {
        const Token *t = peek(&p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "'import' is not supported yet");
    }
    while (peek(&p)->kind != TK_EOF) {
        for (size_t i = 0; i < p.prepend.len; i++)
            stmt_array_push(&tu->globals, p.prepend.data[i]);
        p.prepend.len = 0;
        if (peek(&p)->kind == TK_KW_STRUCT) {
            size_t save = p.pos;
            advance(&p);
            const Token *tag = peek(&p);
            if (tag->kind == TK_IDENT) advance(&p);
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE) {
                if (struct_registry_find(&tu->structs, tag->text)) {
                    die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                           "redefinition of struct '%s'", tag->text);
                }
                StructDef *sd = struct_registry_add(&tu->structs, tag->text, tag->loc);
                parse_struct_body(&p, sd);
                expect_kind(&p, TK_SEMICOLON, "';'");
                continue;
            } else {
                p.pos = save;
            }
        }
        if (peek(&p)->kind == TK_KW_UNION) {
            size_t save = p.pos;
            advance(&p);
            const Token *tag = peek(&p);
            if (tag->kind == TK_IDENT) advance(&p);
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE) {
                if (struct_registry_find(&tu->structs, tag->text)) {
                    die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                           "redefinition of '%s'", tag->text);
                }
                StructDef *sd = struct_registry_add(&tu->structs, tag->text, tag->loc);
                sd->is_union = 1;
                parse_struct_body(&p, sd);
                expect_kind(&p, TK_SEMICOLON, "';'");
                continue;
            } else {
                p.pos = save;
            }
        }
        if (peek(&p)->kind == TK_KW_ENUM) {
            size_t save = p.pos;
            advance(&p);
            const Token *tag = peek(&p);
            if (tag->kind == TK_IDENT) advance(&p);
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE) {
                if (enum_registry_find(&tu->enums, tag->text)) {
                    die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                           "redefinition of enum '%s'", tag->text);
                }
                EnumDef *ed = enum_registry_add(&tu->enums, tag->text, tag->loc);
                parse_enum_body(&p, ed);
                expect_kind(&p, TK_SEMICOLON, "';'");
                continue;
            } else {
                p.pos = save;
            }
        }
        if (peek(&p)->kind == TK_KW_TYPEDEF) {
            Stmt s = parse_typedef_stmt(&p);
            stmt_free(&s);
            continue;
        }
        size_t save = p.pos;
        for (;;) {
            TokenKind tk = peek(&p)->kind;
            if (tk == TK_KW_STATIC || tk == TK_KW_EXTERN || tk == TK_KW_INLINE ||
                tk == TK_KW_CONST || tk == TK_KW_VOLATILE || tk == TK_KW_RESTRICT ||
                tk == TK_KW_SIGNED || tk == TK_KW_UNSIGNED ||
                tk == TK_KW_VOID || tk == TK_KW_INT || tk == TK_KW_CHAR ||
                tk == TK_KW_SHORT || tk == TK_KW_LONG || tk == TK_KW_FLOAT ||
                tk == TK_KW_DOUBLE || tk == TK_KW_BOOL) {
                advance(&p);
            } else if (tk == TK_KW_STRUCT || tk == TK_KW_UNION || tk == TK_KW_ENUM) {
                advance(&p);
                if (peek(&p)->kind == TK_IDENT) advance(&p);
                if (peek(&p)->kind == TK_LBRACE) {
                    int depth = 0;
                    do {
                        if (peek(&p)->kind == TK_LBRACE) depth++;
                        else if (peek(&p)->kind == TK_RBRACE) depth--;
                        advance(&p);
                    } while (depth > 0 && peek(&p)->kind != TK_EOF);
                }
            } else if (tk == TK_IDENT && typedef_registry_find(&tu->typedefs, peek(&p)->text)) {
                advance(&p);
            } else {
                break;
            }
        }
        while (peek(&p)->kind == TK_STAR || peek(&p)->kind == TK_KW_CONST ||
               peek(&p)->kind == TK_KW_VOLATILE || peek(&p)->kind == TK_KW_RESTRICT) advance(&p);
        for (;;) { if (!skip_attribute(&p)) break; }
        int saw_name = 0;
        if (peek(&p)->kind == TK_LPAREN && p.pos + 1 < p.tokens->len &&
            (p.tokens->data[p.pos + 1].kind == TK_STAR || p.tokens->data[p.pos + 1].kind == TK_KW_CONST)) {
            advance(&p);
            while (peek(&p)->kind == TK_STAR || peek(&p)->kind == TK_KW_CONST) advance(&p);
            if (peek(&p)->kind == TK_IDENT) { advance(&p); saw_name = 1; }
            if (peek(&p)->kind == TK_RPAREN) advance(&p);
        } else {
            if (peek(&p)->kind == TK_IDENT) { advance(&p); saw_name = 1; }
        }
        for (;;) { if (!skip_attribute(&p)) break; }
        while (peek(&p)->kind == TK_LBRACKET) {
            advance(&p);
            while (peek(&p)->kind != TK_RBRACKET && peek(&p)->kind != TK_EOF) advance(&p);
            if (peek(&p)->kind == TK_RBRACKET) advance(&p);
        }
        int is_func = (saw_name && peek(&p)->kind == TK_LPAREN);
        p.pos = save;
        if (is_func) {
            FunctionDecl fn = parse_function_decl(&p);
            if (tu->functions.len >= tu->functions.cap) {
                size_t new_cap = tu->functions.cap ? tu->functions.cap * 2 : 4;
                tu->functions.data = realloc(tu->functions.data,
                                             new_cap * sizeof(FunctionDecl));
                tu->functions.cap = new_cap;
            }
            tu->functions.data[tu->functions.len++] = fn;
        } else {
            Stmt s = parse_stmt(&p);
            if (s.kind == ST_BLOCK) {
                stmt_free(&s);
            } else if (s.kind != ST_DECL) {
                die_at(s.loc.file, s.loc.line, s.loc.col,
                       "only variable declarations allowed at file scope (got stmt kind %d, next token '%s')",
                       s.kind, peek(&p)->text ? peek(&p)->text : "NULL");
            } else {
                stmt_array_push(&tu->globals, s);
            }
        }
    }
    for (size_t i = 0; i < p.prepend.len; i++)
        stmt_array_push(&tu->globals, p.prepend.data[i]);
    p.prepend.len = 0;
    stmt_array_free(&p.prepend);
}
