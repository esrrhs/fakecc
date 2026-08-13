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
};typedef enum IROpcode IROpcode;
struct IRInst {
    IROpcode op;
    IRValue dst;
    IRValue a;
    IRValue b;
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
};typedef struct IRInst IRInst;
struct IRInstArray {
    IRInst *data;
    size_t len;
    size_t cap;
};typedef struct IRInstArray IRInstArray;
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
    int ret_is_bool;
    int is_variadic;
    int is_static;
    IRValue sret_value;
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
void ir_generate(const TranslationUnit *tu, IRModule *ir);
const StructRegistry *get_ir_structs(void);
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
struct DomTree {
    int *idom;
    int **df;
    size_t *df_len;
    size_t n;
};typedef struct DomTree DomTree;
void domtree_build(DomTree *dt, const CFG *cfg);
int domtree_dominates(const DomTree *dt, int a, int b);
void domtree_free(DomTree *dt);
struct PhiArg { IRValue val; int pred; };typedef struct PhiArg PhiArg;
struct IRPhi {
    IRValue dst;
    int alloca_slot;
    PhiArg *args;
    size_t num_args;
    size_t cap_args;
    SourceLoc loc;
};typedef struct IRPhi IRPhi;
struct BlockPhiInfo {
    IRPhi *phis;
    size_t num_phis;
    size_t cap_phis;
};typedef struct BlockPhiInfo BlockPhiInfo;
BlockPhiInfo *mem2reg_place_phis(
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    const char **block_stores,
    int *next_value_id);
void block_phi_info_free(BlockPhiInfo *bp, size_t num_blocks);
void mem2reg_rename(
    IRFunction *fn,
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    BlockPhiInfo *block_phi_info,
    char **dead);
void mem2reg_writeback(
    IRFunction *fn,
    const CFG *cfg,
    BlockPhiInfo *block_phi_info,
    char *dead);
int opt_mem2reg(IRFunction *fn);
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
static int block_has_phi_for(const BlockPhiInfo *bp, int alloca_slot) {
    for (size_t i = 0; i < bp->num_phis; i++)
        if (bp->phis[i].alloca_slot == alloca_slot) return 1;
    return 0;
}
static IRPhi *block_add_phi(BlockPhiInfo *bp, int alloca_slot,
                             IRValue dst, SourceLoc loc) {
    if (bp->num_phis >= bp->cap_phis) {
        bp->cap_phis = bp->cap_phis ? bp->cap_phis * 2 : 4;
        bp->phis = xrealloc(bp->phis, bp->cap_phis * sizeof(IRPhi));
    }
    IRPhi *phi = &bp->phis[bp->num_phis++];
    phi->dst = dst;
    phi->alloca_slot = alloca_slot;
    phi->args = ((void*)0);
    phi->num_args = 0;
    phi->cap_args = 0;
    phi->loc = loc;
    return phi;
}
BlockPhiInfo *mem2reg_place_phis(
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    const char **block_stores,
    int *next_value_id)
{
    size_t n = cfg->num;
    BlockPhiInfo *bp = xmalloc(n * sizeof(BlockPhiInfo));
    memset(bp, 0, n * sizeof(BlockPhiInfo));
    if (num_alloca == 0) return bp;
    for (size_t ai = 0; ai < num_alloca; ai++) {
        int slot = alloca_slots[ai];
        char *is_def = calloc(n, 1);
        if (!is_def) continue;
        int *wl = ((void*)0);
        size_t wl_len = 0, wl_cap = 0;
        for (size_t bi = 0; bi < n; bi++) {
            if (block_stores[bi][ai]) {
                is_def[bi] = 1;
                if (wl_len >= wl_cap) {
                    wl_cap = wl_cap ? wl_cap * 2 : 4;
                    wl = xrealloc(wl, wl_cap * sizeof(int));
                }
                wl[wl_len++] = (int)bi;
            }
        }
        while (wl_len > 0) {
            int X = wl[--wl_len];
            for (size_t di = 0; di < dt->df_len[X]; di++) {
                int Y = dt->df[X][di];
                if (!block_has_phi_for(&bp[Y], slot)) {
                    IRValue phi_dst = (*next_value_id)++;
                    SourceLoc phi_loc = {((void*)0), 0, 0};
                    block_add_phi(&bp[Y], slot, phi_dst, phi_loc);
                    if (!is_def[Y]) {
                        is_def[Y] = 1;
                        if (wl_len >= wl_cap) {
                            wl_cap = wl_cap ? wl_cap * 2 : 4;
                            wl = xrealloc(wl, wl_cap * sizeof(int));
                        }
                        wl[wl_len++] = Y;
                    }
                }
            }
        }
        free(wl);
        free(is_def);
    }
    return bp;
}
void block_phi_info_free(BlockPhiInfo *bp, size_t num_blocks) {
    if (!bp) return;
    for (size_t i = 0; i < num_blocks; i++) {
        for (size_t j = 0; j < bp[i].num_phis; j++)
            free(bp[i].phis[j].args);
        free(bp[i].phis);
    }
    free(bp);
}
static void inst_array_init(IRInstArray *a) {
    a->data = ((void*)0); a->len = 0; a->cap = 0;
}
static void inst_array_push(IRInstArray *a, IRInst inst) {
    if (a->len >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 16;
        a->data = xrealloc(a->data, a->cap * sizeof(IRInst));
    }
    a->data[a->len++] = inst;
}
static void inst_array_free_contents(IRInstArray *a) {
    free(a->data);
    a->data = ((void*)0); a->len = 0; a->cap = 0;
}
void mem2reg_writeback(
    IRFunction *fn,
    const CFG *cfg,
    BlockPhiInfo *block_phi_info,
    char *dead)
{
    IRInstArray out;
    inst_array_init(&out);
    for (size_t bi = 0; bi < cfg->num; bi++) {
        const CFGBlock *blk = &cfg->blocks[bi];
        size_t term_idx = blk->end;
        if (blk->end > blk->start) {
            IROpcode last_op = fn->insts.data[blk->end - 1].op;
            if (last_op == IR_BR || last_op == IR_CBR || last_op == IR_RETURN)
                term_idx = blk->end - 1;
        }
        for (size_t i = blk->start; i < term_idx; i++) {
            if (!dead[i])
                inst_array_push(&out, fn->insts.data[i]);
        }
        for (size_t si = 0; si < blk->num_succs; si++) {
            int s = blk->succs[si];
            for (size_t phi_i = 0; phi_i < block_phi_info[s].num_phis; phi_i++) {
                IRPhi *phi = &block_phi_info[s].phis[phi_i];
                for (size_t ai = 0; ai < phi->num_args; ai++) {
                    if (phi->args[ai].pred == (int)bi) {
                        IRInst copy;
                        copy.op = IR_COPY;
                        copy.dst = phi->dst;
                        copy.a = phi->args[ai].val;
                        copy.b = -1;
                        copy.imm = 0;
                        copy.loc = phi->loc;
                        copy.call_name = ((void*)0);
                        copy.call_nargs = 0;
                        copy.width = 8;
                        copy.is_unsigned = 0;
                        inst_array_push(&out, copy);
                        break;
                    }
                }
            }
        }
        if (term_idx < blk->end && !dead[term_idx])
            inst_array_push(&out, fn->insts.data[term_idx]);
    }
    inst_array_free_contents(&fn->insts);
    fn->insts = out;
}
static void phi_add_arg(IRPhi *phi, IRValue val, int pred) {
    if (phi->num_args >= phi->cap_args) {
        phi->cap_args = phi->cap_args ? phi->cap_args * 2 : 4;
        phi->args = xrealloc(phi->args, phi->cap_args * sizeof(PhiArg));
    }
    phi->args[phi->num_args].val = val;
    phi->args[phi->num_args].pred = pred;
    phi->num_args++;
}
struct RenameStack {
    IRValue *vals;
    size_t len;
    size_t cap;
};typedef struct RenameStack RenameStack;
static void rstack_init(RenameStack *s) { s->vals = ((void*)0); s->len = 0; s->cap = 0; }
static void rstack_push(RenameStack *s, IRValue v) {
    if (s->len >= s->cap) {
        s->cap = s->cap ? s->cap * 2 : 4;
        s->vals = xrealloc(s->vals, s->cap * sizeof(IRValue));
    }
    s->vals[s->len++] = v;
}
static IRValue rstack_top(const RenameStack *s) {
    return s->len > 0 ? s->vals[s->len - 1] : -1;
}
static void rstack_pop(RenameStack *s) {
    if (s->len > 0) s->len--;
}
static void rstack_free(RenameStack *s) {
    free(s->vals);
    s->vals = ((void*)0);
    s->len = 0;
    s->cap = 0;
}
static size_t find_alloca_slot(const int *alloca_slots, size_t num_alloca,
                                int slot) {
    for (size_t i = 0; i < num_alloca; i++)
        if (alloca_slots[i] == slot) return i;
    return (size_t)-1;
}
static void mem2reg_rename_dfs(
    int b,
    const IRFunction *fn,
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    BlockPhiInfo *block_phi_info,
    char *d,
    RenameStack *stacks)
{
    const CFGBlock *blk = &cfg->blocks[b];
    size_t *entry_sizes = xmalloc(num_alloca * sizeof(size_t));
    for (size_t ai = 0; ai < num_alloca; ai++)
        entry_sizes[ai] = stacks[ai].len;
    for (size_t phi_i = 0; phi_i < block_phi_info[b].num_phis; phi_i++) {
        IRPhi *phi = &block_phi_info[b].phis[phi_i];
        size_t ai = find_alloca_slot(alloca_slots, num_alloca, phi->alloca_slot);
        if (ai != (size_t)-1)
            rstack_push(&stacks[ai], phi->dst);
    }
    for (size_t i = blk->start; i < blk->end; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LOAD) {
            size_t ai = find_alloca_slot(alloca_slots, num_alloca, inst->a);
            if (ai != (size_t)-1) {
                IRValue reaching = rstack_top(&stacks[ai]);
                if (reaching >= 0) {
                    inst->op = IR_COPY;
                    inst->a = reaching;
                } else {
                    inst->op = IR_CONST;
                    inst->a = -1; inst->b = -1; inst->imm = 0;
                }
            }
        } else if (inst->op == IR_STORE) {
            size_t ai = find_alloca_slot(alloca_slots, num_alloca, inst->a);
            if (ai != (size_t)-1) {
                rstack_push(&stacks[ai], inst->b);
                d[i] = 1;
            }
        }
    }
    for (size_t si = 0; si < blk->num_succs; si++) {
        int s = blk->succs[si];
        for (size_t phi_i = 0; phi_i < block_phi_info[s].num_phis; phi_i++) {
            IRPhi *phi = &block_phi_info[s].phis[phi_i];
            size_t ai = find_alloca_slot(alloca_slots, num_alloca, phi->alloca_slot);
            if (ai != (size_t)-1) {
                IRValue val = rstack_top(&stacks[ai]);
                if (val < 0) val = 0;
                phi_add_arg(phi, val, b);
            }
        }
    }
    for (size_t i = 0; i < dt->n; i++) {
        if (dt->idom[i] == b)
            mem2reg_rename_dfs((int)i, fn, cfg, dt, alloca_slots, num_alloca,
                               block_phi_info, d, stacks);
    }
    for (size_t ai = 0; ai < num_alloca; ai++)
        while (stacks[ai].len > entry_sizes[ai])
            rstack_pop(&stacks[ai]);
    free(entry_sizes);
}
void mem2reg_rename(
    IRFunction *fn,
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    BlockPhiInfo *block_phi_info,
    char **dead)
{
    size_t ninst = fn->insts.len;
    *dead = calloc(ninst, 1);
    if (!*dead) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    char *d = *dead;
    for (size_t i = 0; i < ninst; i++) {
        if (fn->insts.data[i].op != IR_ALLOCA) continue;
        int slot = fn->insts.data[i].dst;
        int is_promotable = 0;
        for (size_t ai = 0; ai < num_alloca; ai++)
            if (alloca_slots[ai] == slot) { is_promotable = 1; break; }
        if (is_promotable) d[i] = 1;
    }
    if (num_alloca == 0) return;
    RenameStack *stacks = xmalloc(num_alloca * sizeof(RenameStack));
    for (size_t ai = 0; ai < num_alloca; ai++) rstack_init(&stacks[ai]);
    mem2reg_rename_dfs(cfg->entry, fn, cfg, dt, alloca_slots, num_alloca,
                       block_phi_info, d, stacks);
    for (size_t ai = 0; ai < num_alloca; ai++) rstack_free(&stacks[ai]);
    free(stacks);
}
int opt_mem2reg(IRFunction *fn)
{
    CFG cfg;
    cfg_build(&cfg, &fn->insts);
    DomTree dt;
    domtree_build(&dt, &cfg);
    const IRInstArray *insts = &fn->insts;
    char *pinned = xmalloc(fn->next_value_id * sizeof(char));
    memset(pinned, 0, fn->next_value_id * sizeof(char));
    for (size_t i = 0; i < insts->len; i++) {
        const IRInst *ii = &insts->data[i];
        if (ii->op == IR_ALLOCA && ii->alloca_bytes > 0 && ii->dst >= 0)
            pinned[ii->dst] = 1;
        if (ii->op == IR_ADDR && ii->a >= 0 && ii->a < fn->next_value_id)
            pinned[ii->a] = 1;
    }
    int *alloca_slots = ((void*)0);
    size_t num_alloca = 0, cap_alloca = 0;
    for (size_t i = 0; i < insts->len; i++) {
        if (insts->data[i].op == IR_ALLOCA) {
            int slot = insts->data[i].dst;
            if (slot >= 0 && pinned[slot]) continue;
            if (num_alloca >= cap_alloca) {
                cap_alloca = cap_alloca ? cap_alloca * 2 : 8;
                alloca_slots = xrealloc(alloca_slots, cap_alloca * sizeof(int));
            }
            alloca_slots[num_alloca++] = slot;
        }
    }
    free(pinned);
    if (num_alloca == 0) {
        free(alloca_slots);
        domtree_free(&dt);
        cfg_free(&cfg);
        return 0;
    }
    char **block_stores = xmalloc(cfg.num * sizeof(char *));
    for (size_t bi = 0; bi < cfg.num; bi++) {
        block_stores[bi] = xmalloc(num_alloca * sizeof(char));
        memset(block_stores[bi], 0, num_alloca * sizeof(char));
    }
    for (size_t i = 0; i < insts->len; i++) {
        if (insts->data[i].op != IR_STORE) continue;
        for (size_t ai = 0; ai < num_alloca; ai++) {
            if (insts->data[i].a == alloca_slots[ai]) {
                for (size_t bi = 0; bi < cfg.num; bi++) {
                    if (i >= cfg.blocks[bi].start && i < cfg.blocks[bi].end) {
                        block_stores[bi][ai] = 1;
                        break;
                    }
                }
                break;
            }
        }
    }
    BlockPhiInfo *bp = mem2reg_place_phis(
        &cfg, &dt, alloca_slots, num_alloca,
        (const char **)block_stores, &fn->next_value_id);
    for (size_t bi = 0; bi < cfg.num; bi++) free(block_stores[bi]);
    free(block_stores);
    char *dead = ((void*)0);
    mem2reg_rename(fn, &cfg, &dt, alloca_slots, num_alloca, bp, &dead);
    mem2reg_writeback(fn, &cfg, bp, dead);
    block_phi_info_free(bp, cfg.num);
    free(alloca_slots);
    free(dead);
    domtree_free(&dt);
    cfg_free(&cfg);
    return (int)num_alloca;
}
