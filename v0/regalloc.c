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
enum IROpcode {
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
    IR_DBG_VALUE,
};typedef enum IROpcode IROpcode;
struct IRInst {
    IROpcode op;
    IRValue dst;
    IRValue a;
    IRValue b;
    int64_t imm;
    SourceLoc loc;
    char *call_name;
    IRValue call_args[32];
    int call_nargs;
    IRValue call_callee;
    int width;
    int is_unsigned;
    int64_t float_imm;
    int is_float;
    int force_stack;
    unsigned char call_arg_on_stack[32];
    int alloca_bytes;
};typedef struct IRInst IRInst;
struct IRInstArray {
    IRInst *data;
    size_t len;
    size_t cap;
};typedef struct IRInstArray IRInstArray;
enum IRDebugVarKind {
    IR_DBG_PARAM = 0,
    IR_DBG_LOCAL = 1
};typedef enum IRDebugVarKind IRDebugVarKind;
struct IRDebugMember {
    char *name;
    int offset;
    int bit_width;
    int bit_offset;
    int type_kind;
    int width;
    int is_unsigned;
    int is_bool;
};typedef struct IRDebugMember IRDebugMember;
struct IRDebugVar {
    char *name;
    SourceLoc loc;
    IRDebugVarKind kind;
    int type_kind;
    int width;
    int is_unsigned;
    int is_bool;
    int array_len;
    char *struct_tag;
    int struct_size;
    IRDebugMember *members;
    int num_members;
    int alloca_ssa;
    int param_idx;
};typedef struct IRDebugVar IRDebugVar;
struct IRFunction {
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
    int ret_reg_n;
    int ret_reg_cls[2];
    int ret_is_bool;
    int is_variadic;
    int is_static;
    IRValue sret_value;
    IRDebugVar *dbg_vars;
    size_t num_dbg_vars;
    size_t cap_dbg_vars;
};typedef struct IRFunction IRFunction;
struct IRFunctionArray {
    IRFunction *data;
    size_t len;
    size_t cap;
};typedef struct IRFunctionArray IRFunctionArray;
struct GlobalFixup {
    int offset;
    char *sym;
};typedef struct GlobalFixup GlobalFixup;
struct IRGlobal {
    char *name;
    int size;
    char *init_bytes;
    int is_readonly;
    int is_static;
    SourceLoc loc;
    GlobalFixup *fixups;
    int num_fixups;
    int cap_fixups;
};typedef struct IRGlobal IRGlobal;
struct IRGlobalArray {
    IRGlobal *data;
    size_t len;
    size_t cap;
};typedef struct IRGlobalArray IRGlobalArray;
struct IRModule {
    IRFunctionArray functions;
    IRGlobalArray globals;
};typedef struct IRModule IRModule;
void ir_module_init(IRModule *m);
void ir_module_free(IRModule *m);
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
void ir_generate(const TranslationUnit *tu, IRModule *ir, int pin_locals);
const StructRegistry *get_ir_structs(void);
enum Reg {
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
    REG_RBX = 3,
    REG_RSP = 4,
    REG_RBP = 5,
    REG_RSI = 6,
    REG_RDI = 7,
    REG_R8 = 8,
    REG_R9 = 9,
    REG_R10 = 10,
    REG_R11 = 11,
    REG_R12 = 12,
    REG_R13 = 13,
    REG_R14 = 14,
    REG_R15 = 15,
    REG_NONE = -1,
};typedef enum Reg Reg;
static const int ALLOCATABLE_REGS[9] = {
    REG_RSI, REG_RDI,
    REG_R8, REG_R9, REG_R10, REG_R11,
    REG_RBX, REG_R12, REG_R13
};
static const int XMM_ALLOCATABLE_REGS[14] = {
     0, 1, 2, 3, 4, 5, 6, 7,
     8, 9, 10, 11, 12, 13
};
struct RAResult {
    int *reg;
    int *spill_slot;
    int num_spill_slots;
    int num_values;
    int stack_size;
};typedef struct RAResult RAResult;
RAResult *reg_alloc(const IRFunction *fn);
RAResult *reg_alloc_xmm(const IRFunction *fn);
void ra_result_free(RAResult *ra);
struct CFGBlock {
    int id;
    int label;
    size_t start;
    size_t end;
    int *preds;
    size_t num_preds;
    int *succs;
    size_t num_succs;
};typedef struct CFGBlock CFGBlock;
struct CFG {
    CFGBlock *blocks;
    size_t num;
    int entry;
};typedef struct CFG CFG;
void cfg_build(CFG *g, const IRInstArray *insts);
void cfg_free(CFG *g);
int cfg_find_label(const CFG *g, int label);
void cfg_link(CFG *g, int from, int to);
int *cfg_rpo(const CFG *g);
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
struct RegClass {
    const int *regs;
    int nregs;
    unsigned caller_saved;
};typedef struct RegClass RegClass;
static const RegClass GP_CLASS = {
    .regs = ALLOCATABLE_REGS,
    .nregs = 9,
    .caller_saved = 0x3Fu,
};
static const RegClass XMM_CLASS = {
    .regs = XMM_ALLOCATABLE_REGS,
    .nregs = 14,
    .caller_saved = ((1u << 14) - 1u),
};
static int value_is_float_class(const IRFunction *fn, int v) {
    if (v < 0) return 0;
    if (!fn->value_is_float || fn->value_meta_cap <= 0) return 0;
    if (v >= fn->value_meta_cap) return 0;
    return fn->value_is_float[v];
}
static int value_is_ld(const IRFunction *fn, int v) {
    if (!value_is_float_class(fn, v)) return 0;
    if (!fn->value_width || fn->value_meta_cap <= 0) return 0;
    if (v >= fn->value_meta_cap) return 0;
    return fn->value_width[v] == 16;
}
static int value_is_xmm_float(const IRFunction *fn, int v) {
    return value_is_float_class(fn, v) && !value_is_ld(fn, v);
}
static int value_in_class(const IRFunction *fn, int v, int float_class) {
    if (float_class) return value_is_xmm_float(fn, v);
    return !value_is_float_class(fn, v);
}
struct LiveInfo {
    int def_point;
    int *use_points;
    size_t num_uses;
    size_t cap_uses;
    int live_start;
    int live_end;
};typedef struct LiveInfo LiveInfo;
static void liv_init(LiveInfo *liv, int n) {
    for (int i = 0; i < n; i++) {
        liv[i].def_point = -1;
        liv[i].use_points = ((void*)0);
        liv[i].num_uses = 0;
        liv[i].cap_uses = 0;
        liv[i].live_start = -1;
        liv[i].live_end = -1;
    }
}
static void liv_free(LiveInfo *liv, int n) {
    for (int i = 0; i < n; i++) free(liv[i].use_points);
    free(liv);
}
static void liv_add_use(LiveInfo *l, int pt) {
    if (l->num_uses >= l->cap_uses) {
        l->cap_uses = l->cap_uses ? l->cap_uses * 2 : 4;
        l->use_points = xrealloc(l->use_points, l->cap_uses * sizeof(int));
    }
    l->use_points[l->num_uses++] = pt;
}
static LiveInfo *compute_liveness(const IRFunction *fn) {
    int nv = fn->next_value_id;
    if (nv <= 0) return ((void*)0);
    LiveInfo *liv = xmalloc(nv * sizeof(LiveInfo));
    liv_init(liv, nv);
    for (size_t i = 0; i < fn->insts.len; i++) {
        const IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LABEL || inst->op == IR_BR ||
            inst->op == IR_DBG_VALUE) continue;
        if (inst->dst >= 0 && inst->dst < nv) {
            liv[inst->dst].def_point = (int)i;
        }
        if (inst->op == IR_CALL && inst->b >= 0 && inst->b < nv)
            liv[inst->b].def_point = (int)i;
        if (inst->a >= 0 && inst->a < nv) liv_add_use(&liv[inst->a], (int)i);
        if (inst->op != IR_CBR && inst->op != IR_CALL &&
            inst->b >= 0 && inst->b < nv) liv_add_use(&liv[inst->b], (int)i);
        if (inst->op == IR_CALL) {
            for (int k = 0; k < inst->call_nargs; k++) {
                IRValue av = inst->call_args[k];
                if (av >= 0 && av < nv) liv_add_use(&liv[av], (int)i);
            }
        }
    }
    for (int v = 0; v < nv; v++) {
        if (liv[v].num_uses == 0) {
            liv[v].live_start = liv[v].def_point >= 0 ? liv[v].def_point : 0;
            liv[v].live_end = liv[v].live_start;
        } else {
            liv[v].live_start = liv[v].def_point >= 0 ? liv[v].def_point
                                : liv[v].use_points[0];
            liv[v].live_end = liv[v].use_points[liv[v].num_uses - 1];
        }
    }
    return liv;
}
struct IGANode {
    int *neighbors;
    size_t degree;
    size_t cap;
};typedef struct IGANode IGANode;
struct InterfGraph {
    IGANode *nodes;
    int n;
};typedef struct InterfGraph InterfGraph;
static void ig_init(InterfGraph *g, int n) {
    g->n = n;
    g->nodes = xmalloc(n * sizeof(IGANode));
    for (int i = 0; i < n; i++) {
        g->nodes[i].neighbors = ((void*)0);
        g->nodes[i].degree = 0;
        g->nodes[i].cap = 0;
    }
}
static void ig_free(InterfGraph *g) {
    for (int i = 0; i < g->n; i++) free(g->nodes[i].neighbors);
    free(g->nodes);
    g->nodes = ((void*)0);
    g->n = 0;
}
static void ig_add_edge(InterfGraph *g, int u, int v) {
    if (u == v) return;
    IGANode *un = &g->nodes[u];
    for (size_t i = 0; i < un->degree; i++)
        if (un->neighbors[i] == v) return;
    if (un->degree >= un->cap) {
        un->cap = un->cap ? un->cap * 2 : 8;
        un->neighbors = xrealloc(un->neighbors, un->cap * sizeof(int));
    }
    un->neighbors[un->degree++] = v;
    IGANode *vn = &g->nodes[v];
    if (vn->degree >= vn->cap) {
        vn->cap = vn->cap ? vn->cap * 2 : 8;
        vn->neighbors = xrealloc(vn->neighbors, vn->cap * sizeof(int));
    }
    vn->neighbors[vn->degree++] = u;
}
struct BitSet {
    uint64_t *w;
    int nv;
    int num_words;
};typedef struct BitSet BitSet;
static void bs_init(BitSet *b, int nv) {
    b->nv = nv;
    b->num_words = (nv + 63) / 64;
    b->w = xmalloc((size_t)b->num_words * sizeof(uint64_t));
    memset(b->w, 0, (size_t)b->num_words * sizeof(uint64_t));
}
static void bs_free(BitSet *b) {
    free(b->w);
    b->w = ((void*)0);
}
static void bs_clear(BitSet *b) {
    memset(b->w, 0, (size_t)b->num_words * sizeof(uint64_t));
}
static int bs_test(const BitSet *b, int v) {
    return (int)((b->w[v >> 6] >> (v & 63)) & 1);
}
static void bs_set(BitSet *b, int v) {
    b->w[v >> 6] |= ((uint64_t)1 << (v & 63));
}
static void bs_clr(BitSet *b, int v) {
    b->w[v >> 6] &= ~((uint64_t)1 << (v & 63));
}
static int bs_or_changed(BitSet *dst, const BitSet *src) {
    int changed = 0;
    for (int i = 0; i < dst->num_words; i++) {
        uint64_t before = dst->w[i];
        uint64_t after = before | src->w[i];
        if (after != before) changed = 1;
        dst->w[i] = after;
    }
    return changed;
}
static void bs_copy(BitSet *dst, const BitSet *src) {
    memcpy(dst->w, src->w, (size_t)dst->num_words * sizeof(uint64_t));
}
static void compute_use_def(const IRFunction *fn, const CFG *cfg,
                            BitSet *use_b, BitSet *def_b) {
    for (size_t bi = 0; bi < cfg->num; bi++) {
        bs_clear(&use_b[bi]);
        bs_clear(&def_b[bi]);
        const CFGBlock *blk = &cfg->blocks[bi];
        for (size_t i = blk->start; i < blk->end; i++) {
            const IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LABEL || inst->op == IR_BR ||
            inst->op == IR_DBG_VALUE) continue;
            if (inst->a >= 0 && inst->a < use_b[bi].nv) {
                if (!bs_test(&def_b[bi], inst->a))
                    bs_set(&use_b[bi], inst->a);
            }
            if (inst->op != IR_CBR && inst->op != IR_CALL &&
                inst->b >= 0 && inst->b < use_b[bi].nv) {
                if (!bs_test(&def_b[bi], inst->b))
                    bs_set(&use_b[bi], inst->b);
            }
            if (inst->op == IR_CALL) {
                for (int k = 0; k < inst->call_nargs; k++) {
                    IRValue av = inst->call_args[k];
                    if (av >= 0 && av < use_b[bi].nv &&
                        !bs_test(&def_b[bi], av))
                        bs_set(&use_b[bi], av);
                }
            }
            if (inst->dst >= 0 && inst->dst < def_b[bi].nv) {
                bs_set(&def_b[bi], inst->dst);
            }
            if (inst->op == IR_CALL && inst->b >= 0 &&
                inst->b < def_b[bi].nv)
                bs_set(&def_b[bi], inst->b);
        }
    }
}
static void compute_live_in_out(const CFG *cfg,
                                const BitSet *use_b, const BitSet *def_b,
                                BitSet *in_b, BitSet *out_b) {
    int changed = 1;
    BitSet tmp;
    bs_init(&tmp, use_b[0].nv);
    while (changed) {
        changed = 0;
        for (int bi = (int)cfg->num - 1; bi >= 0; bi--) {
            const CFGBlock *blk = &cfg->blocks[bi];
            bs_clear(&out_b[bi]);
            for (size_t si = 0; si < blk->num_succs; si++) {
                bs_or_changed(&out_b[bi], &in_b[blk->succs[si]]);
            }
            bs_copy(&tmp, &out_b[bi]);
            for (int wi = 0; wi < tmp.num_words; wi++) {
                tmp.w[wi] &= ~def_b[bi].w[wi];
            }
            for (int wi = 0; wi < tmp.num_words; wi++) {
                tmp.w[wi] |= use_b[bi].w[wi];
            }
            for (int wi = 0; wi < tmp.num_words; wi++) {
                if (tmp.w[wi] != in_b[bi].w[wi]) {
                    changed = 1;
                    break;
                }
            }
            bs_copy(&in_b[bi], &tmp);
        }
    }
    bs_free(&tmp);
}
static void build_interf_graph_cfg(const IRFunction *fn, const CFG *cfg,
                                    int nv, InterfGraph *g,
                                    int *forbid_mask,
                                    int float_class,
                                    const RegClass *cls) {
    ig_init(g, nv);
    for (int v = 0; v < nv; v++) forbid_mask[v] = 0;
    BitSet *use_b = xmalloc(cfg->num * sizeof(BitSet));
    BitSet *def_b = xmalloc(cfg->num * sizeof(BitSet));
    BitSet *in_b = xmalloc(cfg->num * sizeof(BitSet));
    BitSet *out_b = xmalloc(cfg->num * sizeof(BitSet));
    for (size_t bi = 0; bi < cfg->num; bi++) {
        bs_init(&use_b[bi], nv);
        bs_init(&def_b[bi], nv);
        bs_init(&in_b[bi], nv);
        bs_init(&out_b[bi], nv);
    }
    compute_use_def(fn, cfg, use_b, def_b);
    compute_live_in_out(cfg, use_b, def_b, in_b, out_b);
    BitSet live;
    bs_init(&live, nv);
    for (size_t bi = 0; bi < cfg->num; bi++) {
        const CFGBlock *blk = &cfg->blocks[bi];
        bs_copy(&live, &out_b[bi]);
        for (size_t i = blk->end; i > blk->start; i--) {
            const IRInst *inst = &fn->insts.data[i - 1];
        if (inst->op == IR_LABEL || inst->op == IR_BR ||
            inst->op == IR_DBG_VALUE) continue;
            if (inst->op == IR_CALL) {
                for (int _wi = 0; _wi < (&live)->num_words; _wi++) for (uint64_t _w = (&live)->w[_wi], over; _w && ((over = _wi * 64 + __fakecc_ctzll(_w)), 1); _w &= _w - 1) {
                    if ((int)over != inst->dst && (int)over != inst->b &&
                        value_in_class(fn, (int)over, float_class))
                        forbid_mask[over] |= cls->caller_saved;
                }
            }
            if (inst->dst >= 0 && inst->dst < nv &&
                value_in_class(fn, inst->dst, float_class)) {
                if (inst->op != IR_COPY)
                    bs_clr(&live, inst->dst);
            }
            if (inst->op == IR_CALL && inst->b >= 0 && inst->b < nv &&
                value_in_class(fn, inst->b, float_class))
                bs_clr(&live, inst->b);
            if (inst->a >= 0 && inst->a < nv &&
                value_in_class(fn, inst->a, float_class))
                bs_set(&live, inst->a);
            if (inst->op != IR_CBR && inst->op != IR_CALL &&
                inst->b >= 0 && inst->b < nv &&
                value_in_class(fn, inst->b, float_class))
                bs_set(&live, inst->b);
            if (inst->op == IR_CALL) {
                for (int k = 0; k < inst->call_nargs; k++) {
                    IRValue av = inst->call_args[k];
                    if (av >= 0 && av < nv &&
                        value_in_class(fn, av, float_class))
                        bs_set(&live, av);
                }
            }
        }
    }
    {
        int changed = 1;
        while (changed) {
            changed = 0;
            for (size_t bi = 0; bi < cfg->num; bi++) {
                const CFGBlock *blk = &cfg->blocks[bi];
                for (int _wi = 0; _wi < (&out_b[bi])->num_words; _wi++) for (uint64_t _w = (&out_b[bi])->w[_wi], v; _w && ((v = _wi * 64 + __fakecc_ctzll(_w)), 1); _w &= _w - 1) {
                    for (size_t si = 0; si < blk->num_succs; si++) {
                        int s = blk->succs[si];
                        if (bs_test(&in_b[s], (int)v)) {
                            int new_mask = forbid_mask[v] | forbid_mask[s];
                            if (new_mask != forbid_mask[v]) {
                                forbid_mask[v] = new_mask;
                                changed = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    for (size_t bi = 0; bi < cfg->num; bi++) {
        const CFGBlock *blk = &cfg->blocks[bi];
        bs_copy(&live, &out_b[bi]);
        for (size_t i = blk->end; i > blk->start; i--) {
            const IRInst *inst = &fn->insts.data[i - 1];
        if (inst->op == IR_LABEL || inst->op == IR_BR ||
            inst->op == IR_DBG_VALUE) continue;
            if (inst->dst >= 0 && inst->dst < nv &&
                value_in_class(fn, inst->dst, float_class)) {
                for (int _wi = 0; _wi < (&live)->num_words; _wi++) for (uint64_t _w = (&live)->w[_wi], other; _w && ((other = _wi * 64 + __fakecc_ctzll(_w)), 1); _w &= _w - 1) {
                    if (!value_in_class(fn, (int)other, float_class))
                        continue;
                    if ((int)other != inst->dst) {
                        ig_add_edge(g, inst->dst, (int)other);
                    }
                }
                if (inst->op != IR_COPY)
                    bs_clr(&live, inst->dst);
            }
            if (inst->op == IR_CALL && inst->b >= 0 && inst->b < nv &&
                value_in_class(fn, inst->b, float_class)) {
                for (int _wi = 0; _wi < (&live)->num_words; _wi++) for (uint64_t _w = (&live)->w[_wi], other; _w && ((other = _wi * 64 + __fakecc_ctzll(_w)), 1); _w &= _w - 1) {
                    if (!value_in_class(fn, (int)other, float_class))
                        continue;
                    if ((int)other != inst->b)
                        ig_add_edge(g, inst->b, (int)other);
                }
                bs_clr(&live, inst->b);
            }
            if (inst->a >= 0 && inst->a < nv &&
                value_in_class(fn, inst->a, float_class))
                bs_set(&live, inst->a);
            if (inst->op != IR_CBR && inst->op != IR_CALL &&
                inst->b >= 0 && inst->b < nv &&
                value_in_class(fn, inst->b, float_class))
                bs_set(&live, inst->b);
            if (inst->op == IR_CALL) {
                for (int k = 0; k < inst->call_nargs; k++) {
                    IRValue av = inst->call_args[k];
                    if (av >= 0 && av < nv &&
                        value_in_class(fn, av, float_class))
                        bs_set(&live, av);
                }
            }
        }
    }
    bs_free(&live);
    for (size_t bi = 0; bi < cfg->num; bi++) {
        bs_free(&use_b[bi]);
        bs_free(&def_b[bi]);
        bs_free(&in_b[bi]);
        bs_free(&out_b[bi]);
    }
    free(use_b); free(def_b); free(in_b); free(out_b);
}
static int *compute_mcs_order(const InterfGraph *g) {
    int n = g->n;
    int *order = xmalloc(n * sizeof(int));
    int *weight = xmalloc(n * sizeof(int));
    int *picked = xmalloc(n * sizeof(int));
    memset(weight, 0, n * sizeof(int));
    memset(picked, 0, n * sizeof(int));
    for (int pos = n - 1; pos >= 0; pos--) {
        int best = -1, best_w = -1;
        for (int v = 0; v < n; v++) {
            if (!picked[v] && weight[v] > best_w) {
                best = v;
                best_w = weight[v];
            }
        }
        if (best < 0) {
            for (int v = 0; v < n; v++) {
                if (!picked[v]) { best = v; break; }
            }
        }
        picked[best] = 1;
        order[pos] = best;
        for (size_t j = 0; j < g->nodes[best].degree; j++) {
            int w = g->nodes[best].neighbors[j];
            if (!picked[w]) weight[w]++;
        }
    }
    free(weight);
    free(picked);
    return order;
}
static int estimate_loop_depth(int block_id, const CFG *cfg) {
    int depth = 0;
    const CFGBlock *b = &cfg->blocks[block_id];
    for (size_t s = 0; s < b->num_succs; s++) {
        int succ = b->succs[s];
        for (size_t p = 0; p < cfg->blocks[succ].num_preds; p++) {
            if (succ <= block_id) { depth++; break; }
        }
    }
    return depth > 0 ? 1 : 0;
}
static int find_block_for_inst(const CFG *cfg, int inst_idx) {
    for (size_t bi = 0; bi < cfg->num; bi++) {
        if ((size_t)inst_idx >= cfg->blocks[bi].start &&
            (size_t)inst_idx < cfg->blocks[bi].end)
            return (int)bi;
    }
    return 0;
}
static int compute_spill_cost(int v, const LiveInfo *liv, const CFG *cfg) {
    const LiveInfo *l = &liv[v];
    int cost = 0;
    for (size_t i = 0; i < l->num_uses; i++) {
        int blk = find_block_for_inst(cfg, l->use_points[i]);
        cost += 1 + estimate_loop_depth(blk, cfg) * 10;
    }
    if (l->def_point >= 0) {
        int blk = find_block_for_inst(cfg, l->def_point);
        cost += 1 + estimate_loop_depth(blk, cfg) * 10;
    }
    return cost > 0 ? cost : 1;
}
static void greedy_color(const InterfGraph *g, const int *order,
                         const LiveInfo *liv, const CFG *cfg,
                         const int *forbid_mask,
                         int *colors, int *spill_slots, int *num_spills,
                         const RegClass *cls) {
    int n = g->n;
    int k = cls->nregs;
    int *spill_cost = xmalloc(n * sizeof(int));
    for (int v = 0; v < n; v++)
        spill_cost[v] = compute_spill_cost(v, liv, cfg);
    for (int v = 0; v < n; v++) colors[v] = -1;
    int next_spill = 0;
    for (int i = 0; i < n; i++) {
        int v = order[i];
        if (g->nodes[v].degree == 0 && liv[v].num_uses == 0 && liv[v].def_point < 0) {
            colors[v] = -2;
            spill_slots[v] = next_spill++;
            continue;
        }
        int used = forbid_mask[v];
        for (size_t j = 0; j < g->nodes[v].degree; j++) {
            int w = g->nodes[v].neighbors[j];
            if (colors[w] >= 0 && colors[w] < k)
                used |= (1 << colors[w]);
        }
        int c;
        for (c = 0; c < k; c++)
            if (!(used & (1 << c))) break;
        if (c < k) {
            colors[v] = c;
        } else {
            int color_count[32] = {0};
            for (size_t j = 0; j < g->nodes[v].degree; j++) {
                int w = g->nodes[v].neighbors[j];
                if (colors[w] >= 0 && colors[w] < k &&
                    !(forbid_mask[v] & (1 << colors[w])))
                    color_count[colors[w]]++;
            }
            int victim = -1, victim_cost = 0x7fffffff;
            for (size_t j = 0; j < g->nodes[v].degree; j++) {
                int w = g->nodes[v].neighbors[j];
                if (colors[w] < 0 || colors[w] >= k) continue;
                if (forbid_mask[v] & (1 << colors[w])) continue;
                if (color_count[colors[w]] != 1) continue;
                if (spill_cost[w] < victim_cost) {
                    victim = w;
                    victim_cost = spill_cost[w];
                }
            }
            if (victim >= 0 && victim_cost < spill_cost[v]) {
                colors[v] = colors[victim];
                colors[victim] = -2;
                spill_slots[victim] = next_spill++;
            } else {
                colors[v] = -2;
                spill_slots[v] = next_spill++;
            }
        }
    }
    *num_spills = next_spill;
    free(spill_cost);
}
static void coalesce_copies(IRFunction *fn, int *colors) {
    (void)fn;
    (void)colors;
}
static RAResult *ra_alloc_class(const IRFunction *fn, int float_class,
                                const RegClass *cls) {
    int nv = fn->next_value_id;
    if (nv <= 0) return ((void*)0);
    {
        int any = 0;
        for (int v = 0; v < nv; v++) {
            if (value_in_class(fn, v, float_class)) { any = 1; break; }
        }
        if (!any) return ((void*)0);
    }
    LiveInfo *liv = compute_liveness(fn);
    if (!liv) return ((void*)0);
    CFG cfg;
    cfg_build(&cfg, &fn->insts);
    InterfGraph g;
    int *forbid_mask = xmalloc(nv * sizeof(int));
    build_interf_graph_cfg(fn, &cfg, nv, &g, forbid_mask,
                            float_class, cls);
    int *order = compute_mcs_order(&g);
    int *colors = xmalloc(nv * sizeof(int));
    int *spill_slots = xmalloc(nv * sizeof(int));
    memset(spill_slots, 0, nv * sizeof(int));
    int num_spills = 0;
    greedy_color(&g, order, liv, &cfg, forbid_mask,
                 colors, spill_slots, &num_spills, cls);
    free(forbid_mask);
    for (int v = 0; v < nv; v++) {
        if (!value_in_class(fn, v, float_class)) {
            colors[v] = REG_NONE;
        } else if (colors[v] >= 0 && colors[v] < cls->nregs) {
            colors[v] = cls->regs[colors[v]];
        } else {
        }
    }
    coalesce_copies((IRFunction *)fn, colors);
    RAResult *ra = xmalloc(sizeof(RAResult));
    ra->reg = colors;
    ra->spill_slot = spill_slots;
    ra->num_spill_slots = num_spills;
    ra->num_values = nv;
    int slots = num_spills;
    if (slots % 2 != 0) slots++;
    ra->stack_size = 8 * slots;
    free(order);
    ig_free(&g);
    liv_free(liv, nv);
    cfg_free(&cfg);
    return ra;
}
RAResult *reg_alloc(const IRFunction *fn) {
    return ra_alloc_class(fn, 0, &GP_CLASS);
}
RAResult *reg_alloc_xmm(const IRFunction *fn) {
    return ra_alloc_class(fn, 1, &XMM_CLASS);
}
void ra_result_free(RAResult *ra) {
    if (!ra) return;
    free(ra->reg);
    free(ra->spill_slot);
    free(ra);
}
