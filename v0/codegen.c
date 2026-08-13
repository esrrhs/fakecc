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
struct EmitSymbol {
    char *name;
    uint8_t binding;
    uint8_t type;
    uint16_t shndx;
    size_t value;
    size_t size;
};typedef struct EmitSymbol EmitSymbol;
struct EmitReloc {
    size_t offset;
    uint32_t type;
    uint32_t sym;
    int32_t addend;
};typedef struct EmitReloc EmitReloc;
struct EmitModule {
    Buffer text;
    Buffer rodata;
    Buffer data;
    size_t bss_size;
    EmitSymbol *syms;
    size_t num_syms;
    size_t cap_syms;
    EmitReloc *relocs;
    size_t num_relocs;
    size_t cap_relocs;
    EmitReloc *data_relocs;
    size_t num_data_relocs;
    size_t cap_data_relocs;
};typedef struct EmitModule EmitModule;
void emit_module_init(EmitModule *m);
void emit_module_free(EmitModule *m);
int emit_module_add_symbol(EmitModule *m, const char *name,
                            uint8_t binding, uint8_t type,
                            uint16_t shndx, size_t value, size_t size);
int emit_module_find_symbol(EmitModule *m, const char *name);
int emit_module_add_undefined(EmitModule *m, const char *name);
void emit_module_add_reloc(EmitModule *m, size_t offset, uint32_t type,
                           int sym, int32_t addend);
void emit_module_add_data_reloc(EmitModule *m, size_t offset, uint32_t type,
                                int sym, int32_t addend);
void emit_obj(const EmitModule *m, const char *path);
int emit_obj_read(const char *path, EmitModule *m);
void emit_link(EmitModule **mods, size_t n, const char *path);
void emit_elf(const EmitModule *m, const char *path);
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
void codegen(const IRModule *ir, EmitModule *out);
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
static void emit_byte(Buffer *b, uint8_t val) {
    buffer_append(b, (const char *)&val, 1);
}
static void emit_int32(Buffer *b, int32_t val) {
    buffer_append(b, (const char *)&val, 4);
}
static void emit_modrm(Buffer *b, int mod, int reg, int rm) {
    emit_byte(b, (uint8_t)(((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7)));
}
static void emit_rex_wrb(Buffer *b, int w, int r_reg, int rm_reg) {
    int R = (r_reg >> 3) & 1;
    int B = (rm_reg >> 3) & 1;
    emit_byte(b, (uint8_t)(0x40 | (w << 3) | (R << 2) | B));
}
static void emit_rex_w(Buffer *b) { emit_byte(b, 0x48); }
static void emit_mov_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x89);
    emit_modrm(b, 3, src, dst);
}
static void emit_add_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x01);
    emit_modrm(b, 3, src, dst);
}
static void emit_sub_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x29);
    emit_modrm(b, 3, src, dst);
}
static void emit_imul_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0xAF);
    emit_modrm(b, 3, dst, src);
}
static void emit_mov_imm(Buffer *b, int dst_reg, int32_t imm) {
    emit_rex_wrb(b, 1, 0, dst_reg);
    emit_byte(b, 0xC7);
    emit_modrm(b, 3, 0, dst_reg);
    emit_int32(b, imm);
}
static void emit_neg_r(Buffer *b, int dst_reg) {
    emit_rex_wrb(b, 1, 0, dst_reg);
    emit_byte(b, 0xF7);
    emit_modrm(b, 3, 3, dst_reg);
}
static void emit_not_r(Buffer *b, int dst_reg) {
    emit_rex_wrb(b, 1, 0, dst_reg);
    emit_byte(b, 0xF7);
    emit_modrm(b, 3, 2, dst_reg);
}
static void emit_and_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x21);
    emit_modrm(b, 3, src, dst);
}
static void emit_or_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x09);
    emit_modrm(b, 3, src, dst);
}
static void emit_bitxor_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x31);
    emit_modrm(b, 3, src, dst);
}
static void emit_shl_rcx(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, 0, dst);
    emit_byte(b, 0xD3);
    emit_modrm(b, 3, 4, dst);
}
static void emit_shr_rcx(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, 0, dst);
    emit_byte(b, 0xD3);
    emit_modrm(b, 3, 5, dst);
}
static void emit_sar_rcx(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, 0, dst);
    emit_byte(b, 0xD3);
    emit_modrm(b, 3, 7, dst);
}
static void emit_cqto(Buffer *b) { emit_rex_w(b); emit_byte(b, 0x99); }
static void emit_idiv_rcx(Buffer *b) {
    emit_rex_w(b);
    emit_byte(b, 0xF7);
    emit_byte(b, 0xF9);
}
static void emit_cmp_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x39);
    emit_modrm(b, 3, src, dst);
}
static void emit_cmp_imm32(Buffer *b, int reg, int32_t imm) {
    emit_rex_wrb(b, 1, 0, reg);
    emit_byte(b, 0x81);
    emit_modrm(b, 3, 7, reg);
    emit_int32(b, imm);
}
static void emit_add_imm32(Buffer *b, int reg, int32_t imm) {
    emit_rex_wrb(b, 1, 0, reg);
    emit_byte(b, 0x81);
    emit_modrm(b, 3, 0, reg);
    emit_int32(b, imm);
}
static void patch_rel32(Buffer *b, size_t patch_off, size_t target_off) {
    int32_t rel = (int32_t)((int64_t)target_off - (int64_t)(patch_off + 4));
    memcpy(b->data + patch_off, &rel, 4);
}
static void emit_test_rr(Buffer *b, int r) {
    emit_rex_wrb(b, 1, r, r);
    emit_byte(b, 0x85);
    emit_modrm(b, 3, r, r);
}
static void emit_setcc_r(Buffer *b, uint8_t cc_opcode, int r) {
    uint8_t rex = 0x40 | ((r & 8) >> 3);
    emit_byte(b, rex);
    emit_byte(b, 0x0F);
    emit_byte(b, cc_opcode);
    emit_modrm(b, 3, 0, r & 7);
}
static void emit_xor_rr(Buffer *b, int r) {
    emit_rex_wrb(b, 1, r, r);
    emit_byte(b, 0x31);
    emit_modrm(b, 3, r, r);
}
static void emit_sse_mov_rr(Buffer *b, int dst, int src) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    emit_modrm(b, 3, dst & 7, src & 7);
}
static void emit_sse_load_spill(Buffer *b, int dst, int off) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, REG_RBP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, dst & 7, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, dst & 7, REG_RBP);
        emit_int32(b, off);
    }
}
static void emit_sse_store_spill(Buffer *b, int src, int off) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, src, REG_RBP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x11);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, src & 7, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, src & 7, REG_RBP);
        emit_int32(b, off);
    }
}
static void emit_sse_arith(Buffer *b, int op, int dst, int src, int is_float) {
    emit_byte(b, is_float ? 0xF3 : 0xF2);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, op);
    emit_modrm(b, 3, dst & 7, src & 7);
}
static void emit_sse_ucomi(Buffer *b, int a, int b_xmm, int is_float) {
    if (!is_float) emit_byte(b, 0x66);
    emit_rex_wrb(b, 0, a, b_xmm);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2E);
    emit_modrm(b, 3, a & 7, b_xmm & 7);
}
static void emit_sse_cvtsi2sd(Buffer *b, int xmm, int gp, int is_64) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, is_64 ? 1 : 0, xmm, gp);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2A);
    emit_modrm(b, 3, xmm & 7, gp & 7);
}
static void emit_sse_cvtsd2si(Buffer *b, int gp, int xmm, int is_64) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, is_64 ? 1 : 0, gp, xmm);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2C);
    emit_modrm(b, 3, gp & 7, xmm & 7);
}
static void emit_sse_cvtss2si(Buffer *b, int gp, int xmm, int is_64) {
    emit_byte(b, 0xF3);
    emit_rex_wrb(b, is_64 ? 1 : 0, gp, xmm);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2C);
    emit_modrm(b, 3, gp & 7, xmm & 7);
}
static void emit_sse_cvtss2sd(Buffer *b, int dst, int src) {
    emit_byte(b, 0xF3);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x5A);
    emit_modrm(b, 3, dst & 7, src & 7);
}
static void emit_sse_cvtsd2ss(Buffer *b, int dst, int src) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x5A);
    emit_modrm(b, 3, dst & 7, src & 7);
}
static void emit_movq_xmm_gp(Buffer *b, int xmm, int gp) {
    emit_byte(b, 0x66);
    emit_rex_wrb(b, 1, xmm, gp);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x6E);
    emit_modrm(b, 3, xmm & 7, gp & 7);
}
static void emit_store_rsp_off(Buffer *b, int reg, int off) {
    emit_rex_wrb(b, 1, reg, REG_RSP);
    emit_byte(b, 0x89);
    if (off >= 0 && off <= 127) {
        emit_modrm(b, 1, reg, 4);
        emit_byte(b, 0x24);
        emit_byte(b, (uint8_t)off);
    } else {
        emit_modrm(b, 2, reg, 4);
        emit_byte(b, 0x24);
        emit_int32(b, off);
    }
}
static void emit_sse_store_rsp(Buffer *b, int src) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, src, REG_RSP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x11);
    emit_modrm(b, 0, src & 7, 4);
    emit_byte(b, 0x24);
}
static void emit_sse_store_rsp_off(Buffer *b, int dst, int off) {
    emit_rex_wrb(b, 0, dst, REG_RSP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x29);
    if (off >= 0 && off <= 127) {
        emit_modrm(b, 1, dst, 4);
        emit_byte(b, 0x24);
        emit_byte(b, (uint8_t)off);
    } else {
        emit_modrm(b, 2, dst, 4);
        emit_byte(b, 0x24);
        emit_int32(b, off);
    }
}
static void emit_sse_load_base_off(Buffer *b, int dst, int base, int off) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, base);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    if (off >= -128 && off <= 127) {
        if (base == REG_RSP) {
            emit_modrm(b, 1, dst, 4);
            emit_byte(b, 0x24);
            emit_byte(b, (uint8_t)(off & 0xFF));
        } else {
            emit_modrm(b, 1, dst, base);
            emit_byte(b, (uint8_t)(off & 0xFF));
        }
    } else {
        if (base == REG_RSP) {
            emit_modrm(b, 2, dst, 4);
            emit_byte(b, 0x24);
        } else {
            emit_modrm(b, 2, dst, base);
        }
        emit_int32(b, off);
    }
}
static void emit_sse_load_rsp(Buffer *b, int dst) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, REG_RSP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    emit_modrm(b, 0, dst & 7, 4);
    emit_byte(b, 0x24);
}
static size_t emit_jmp_rel32(Buffer *b) {
    emit_byte(b, 0xE9);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}
static size_t emit_jcc_rel32(Buffer *b, uint8_t cc_opcode) {
    emit_byte(b, 0x0F);
    emit_byte(b, cc_opcode);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}
static void emit_push_r(Buffer *b, int r) {
    if (r >= 8) emit_byte(b, 0x41);
    emit_byte(b, (uint8_t)(0x50 | (r & 7)));
}
static void emit_pop_r(Buffer *b, int r) {
    if (r >= 8) emit_byte(b, 0x41);
    emit_byte(b, (uint8_t)(0x58 | (r & 7)));
}
static void emit_sub_rsp_imm32(Buffer *b, int32_t imm) {
    emit_rex_w(b);
    emit_byte(b, 0x81);
    emit_byte(b, 0xEC);
    emit_int32(b, imm);
}
static void emit_add_rsp_imm32(Buffer *b, int32_t imm) {
    emit_rex_w(b);
    emit_byte(b, 0x81);
    emit_byte(b, 0xC4);
    emit_int32(b, imm);
}
static size_t emit_call_rel32(Buffer *b) {
    emit_byte(b, 0xE8);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}
static void emit_indirect_call(Buffer *b, int reg) {
    if (reg >= 8) emit_byte(b, 0x41);
    emit_byte(b, 0xFF);
    emit_modrm(b, 3, 2, reg & 7);
}
static void emit_x87_fldtRCX(Buffer *b) {
    emit_byte(b, 0xDB);
    emit_modrm(b, 0, 5, REG_RCX & 7);
}
static void emit_x87_fstptRCX(Buffer *b) {
    emit_byte(b, 0xDB);
    emit_modrm(b, 0, 7, REG_RCX & 7);
}
static void emit_x87_arith_pop(Buffer *b, int op) {
    emit_byte(b, 0xDE);
    emit_byte(b, (uint8_t)op);
}
static void emit_x87_fstpt_rsp(Buffer *b) {
    emit_byte(b, 0xDB);
    emit_modrm(b, 0, 7, 4);
    emit_byte(b, 0x24);
}
static void emit_x87_fcomip(Buffer *b) {
    emit_byte(b, 0xDF);
    emit_byte(b, 0xF1);
}
static void emit_ld_addr(Buffer *b, int reg, int ld_off);
static void emit_store_base_off(Buffer *b, int base, int reg, int off);
static void emit_load_base_off(Buffer *b, int dst, int base, int off);
static void emit_ld_load(Buffer *b, int v, const int *ld_off) {
    emit_ld_addr(b, REG_RCX, ld_off[v]);
    emit_x87_fldtRCX(b);
}
static void emit_ld_store(Buffer *b, int v, const int *ld_off) {
    emit_ld_addr(b, REG_RCX, ld_off[v]);
    emit_x87_fstptRCX(b);
}
static void emit_ld_from_gp_int(Buffer *b, int src, int dst, const int *ld_off) {
    emit_ld_addr(b, REG_RCX, ld_off[dst]);
    emit_store_base_off(b, REG_RCX, src, 0);
    emit_byte(b, 0xDB); emit_modrm(b, 0, 0, REG_RCX & 7);
    emit_x87_fstptRCX(b);
}
static const int SYSV_ARG_REGS[6] = {
    REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9
};
static int curr_cs_count = 0;
static int spill_offset(int slot) { return -8 * (slot + 1 + curr_cs_count); }
static void emit_store_spill(Buffer *b, int reg, int off) {
    emit_rex_wrb(b, 1, reg, REG_RBP);
    emit_byte(b, 0x89);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}
static void emit_load_spill(Buffer *b, int reg, int off) {
    emit_rex_wrb(b, 1, reg, REG_RBP);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}
static void emit_lea_rbp(Buffer *b, int dst, int off) {
    emit_rex_wrb(b, 1, dst, REG_RBP);
    emit_byte(b, 0x8D);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, dst, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, dst, REG_RBP);
        emit_int32(b, off);
    }
}
static size_t emit_lea_rip(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, dst, 0 );
    emit_byte(b, 0x8D);
    emit_modrm(b, 0, dst & 7, 5);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}
static size_t emit_load_rip(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, dst, 0 );
    emit_byte(b, 0x8B);
    emit_modrm(b, 0, dst & 7, 5);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}
static void emit_modrm_indirect(Buffer *b, int reg_field, int base) {
    int rm = base & 7;
    if (rm == 4) {
        emit_modrm(b, 0, reg_field & 7, 4);
        emit_byte(b, 0x24);
    } else if (rm == 5) {
        emit_modrm(b, 1, reg_field & 7, 5);
        emit_byte(b, 0);
    } else {
        emit_modrm(b, 0, reg_field & 7, rm);
    }
}
static void emit_sse_load_via_ptr(Buffer *b, int dst, int ptr, int is_float) {
    emit_byte(b, is_float ? 0xF3 : 0xF2);
    emit_rex_wrb(b, 0, dst, ptr);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    emit_modrm_indirect(b, dst, ptr);
}
static void emit_sse_store_via_ptr(Buffer *b, int ptr, int src, int is_float) {
    emit_byte(b, is_float ? 0xF3 : 0xF2);
    emit_rex_wrb(b, 0, src, ptr);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x11);
    emit_modrm_indirect(b, src, ptr);
}
static void emit_store_via_ptr(Buffer *b, int ptr, int src, int width) {
    switch (width) {
    case 1: {
        uint8_t rex = 0x40 | ((src & 8) >> 1) | ((ptr & 8) >> 3);
        emit_byte(b, rex);
        emit_byte(b, 0x88);
        emit_modrm_indirect(b, src, ptr);
        break;
    }
    case 2:
        emit_byte(b, 0x66);
        emit_rex_wrb(b, 0, src, ptr);
        emit_byte(b, 0x89);
        emit_modrm_indirect(b, src, ptr);
        break;
    case 4:
        if (src >= 8 || ptr >= 8) emit_rex_wrb(b, 0, src, ptr);
        emit_byte(b, 0x89);
        emit_modrm_indirect(b, src, ptr);
        break;
    case 8:
    default:
        emit_rex_wrb(b, 1, src, ptr);
        emit_byte(b, 0x89);
        emit_modrm_indirect(b, src, ptr);
        break;
    }
}
static void emit_load_via_ptr(Buffer *b, int dst, int ptr, int width, int is_unsigned) {
    switch (width) {
    case 1:
        emit_rex_wrb(b, 1, dst, ptr);
        emit_byte(b, 0x0F);
        emit_byte(b, is_unsigned ? 0xB6 : 0xBE);
        emit_modrm_indirect(b, dst, ptr);
        break;
    case 2:
        emit_rex_wrb(b, 1, dst, ptr);
        emit_byte(b, 0x0F);
        emit_byte(b, is_unsigned ? 0xB7 : 0xBF);
        emit_modrm_indirect(b, dst, ptr);
        break;
    case 4:
        if (is_unsigned) {
            if (dst >= 8 || ptr >= 8) emit_rex_wrb(b, 0, dst, ptr);
            emit_byte(b, 0x8B);
        } else {
            emit_rex_wrb(b, 1, dst, ptr);
            emit_byte(b, 0x63);
        }
        emit_modrm_indirect(b, dst, ptr);
        break;
    case 8:
    default:
        emit_rex_wrb(b, 1, dst, ptr);
        emit_byte(b, 0x8B);
        emit_modrm_indirect(b, dst, ptr);
        break;
    }
}
static void emit_movsx_rr(Buffer *b, int dst, int src, int src_w) {
    if (src_w == 4) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x63);
        emit_modrm(b, 3, dst, src);
    } else if (src_w == 2) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xBF);
        emit_modrm(b, 3, dst, src);
    } else if (src_w == 1) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xBE);
        emit_modrm(b, 3, dst, src);
    } else {
        if (dst != src) emit_mov_rr(b, dst, src);
    }
}
static void emit_movzx_rr(Buffer *b, int dst, int src, int src_w) {
    if (src_w == 4) {
        if (dst >= 8 || src >= 8) emit_rex_wrb(b, 0, src, dst);
        emit_byte(b, 0x89);
        emit_modrm(b, 3, src, dst);
    } else if (src_w == 2) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xB7);
        emit_modrm(b, 3, dst, src);
    } else if (src_w == 1) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xB6);
        emit_modrm(b, 3, dst, src);
    } else {
        if (dst != src) emit_mov_rr(b, dst, src);
    }
}
static void mask_to_width(Buffer *b, int reg, int width, int is_unsigned) {
    if (width >= 8 || width <= 0) return;
    if (is_unsigned)
        emit_movzx_rr(b, reg, reg, width);
    else
        emit_movsx_rr(b, reg, reg, width);
}
static int old_slot(int v) { return -(8 * (v + 1)); }
static void old_load(Buffer *b, int v, int reg) {
    int off = old_slot(v);
    emit_rex_wrb(b, 1, reg, REG_RBP);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}
static void old_store(Buffer *b, int v, int reg) {
    int off = old_slot(v);
    emit_rex_wrb(b, 1, reg, REG_RBP);
    emit_byte(b, 0x89);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}
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
static void emit_ld_addr(Buffer *b, int reg, int ld_off) {
    emit_lea_rbp(b, reg, ld_off);
}
static void ensure_reg(Buffer *b, int v, int dst_reg, const RAResult *ra) {
    if (!ra || v < 0 || v >= ra->num_values) {
        old_load(b, v, dst_reg);
        return;
    }
    int vr = ra->reg[v];
    if (vr == dst_reg) return;
    if (vr >= 0 && vr < 16) {
        emit_mov_rr(b, dst_reg, vr);
    } else {
        emit_load_spill(b, dst_reg, spill_offset(ra->spill_slot[v]));
    }
}
static void spill_if_needed(Buffer *b, int v, int src_reg, const RAResult *ra) {
    if (!ra || v < 0 || v >= ra->num_values) {
        old_store(b, v, src_reg);
        return;
    }
    if (ra->reg[v] >= 0 && ra->reg[v] < 16) return;
    emit_store_spill(b, src_reg, spill_offset(ra->spill_slot[v]));
}
static int spill_offset_xmm(int slot, int gp_spill_area) {
    return -(gp_spill_area + 8 * (slot + 1 + curr_cs_count));
}
static void ensure_reg_xmm(Buffer *b, int v, int dst_xmm, const RAResult *ra_xmm,
                           int gp_spill_area) {
    if (!ra_xmm || v < 0 || v >= ra_xmm->num_values) {
        fprintf(stderr, "fakecc: float value %d has no XMM home\n", v);
        exit(1);
    }
    int vr = ra_xmm->reg[v];
    if (vr == dst_xmm) return;
    if (vr >= 0 && vr < 16) {
        emit_sse_mov_rr(b, dst_xmm, vr);
    } else {
        emit_sse_load_spill(b, dst_xmm, spill_offset_xmm(ra_xmm->spill_slot[v],
                                                          gp_spill_area));
    }
}
static void spill_if_needed_xmm(Buffer *b, int v, int src_xmm,
                                const RAResult *ra_xmm, int gp_spill_area) {
    if (!ra_xmm || v < 0 || v >= ra_xmm->num_values) return;
    if (ra_xmm->reg[v] >= 0 && ra_xmm->reg[v] < 16) return;
    emit_sse_store_spill(b, src_xmm,
                         spill_offset_xmm(ra_xmm->spill_slot[v], gp_spill_area));
}
static void emit_float_const(Buffer *b, int xmm_dst, int64_t bits) {
    emit_rex_w(b);
    emit_byte(b, 0xB8);
    for (int i = 0; i < 8; i++)
        emit_byte(b, (uint8_t)(bits >> (i * 8)));
    emit_movq_xmm_gp(b, xmm_dst, REG_RAX);
}
static void emit_cmp_produce(Buffer *b, int a_reg, int b_reg, int dst_reg,
                             uint8_t cc_opcode) {
    emit_xor_rr(b, dst_reg);
    emit_cmp_rr(b, a_reg, b_reg);
    emit_setcc_r(b, cc_opcode, dst_reg);
}
static uint8_t ir_cmp_to_setcc(int ir_op, int is_unsigned) {
    if (!is_unsigned) {
        switch (ir_op) {
        case IR_EQ: return 0x94;
        case IR_NE: return 0x95;
        case IR_LT: return 0x9C;
        case IR_LE: return 0x9E;
        case IR_GT: return 0x9F;
        case IR_GE: return 0x9D;
        default: return 0x94;
        }
    } else {
        switch (ir_op) {
        case IR_EQ: return 0x94;
        case IR_NE: return 0x95;
        case IR_LT: return 0x92;
        case IR_LE: return 0x96;
        case IR_GT: return 0x97;
        case IR_GE: return 0x93;
        default: return 0x94;
        }
    }
}
struct Patch {
    size_t patch_off;
    int label;
    size_t after_off;
};typedef struct Patch Patch;
struct CallPatch {
    size_t patch_off;
    char *callee;
    size_t after_off;
};typedef struct CallPatch CallPatch;
struct FnAddrPatch {
    size_t patch_off;
    char *fn_name;
};typedef struct FnAddrPatch FnAddrPatch;
static void collect_callee_saved(const RAResult *ra, int used[3]) {
    used[0] = used[1] = used[2] = 0;
    if (!ra) return;
    for (int v = 0; v < ra->num_values; v++) {
        int r = ra->reg[v];
        if (r == REG_RBX) used[0] = 1;
        else if (r == REG_R12) used[1] = 1;
        else if (r == REG_R13) used[2] = 1;
    }
}
static void emit_store_base_off(Buffer *b, int base, int reg, int off) {
    emit_rex_wrb(b, 1, reg, base);
    emit_byte(b, 0x89);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, base);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, base);
        emit_int32(b, off);
    }
}
static void emit_load_base_off(Buffer *b, int dst, int base, int off) {
    emit_rex_wrb(b, 1, dst, base);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, dst, base);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, dst, base);
        emit_int32(b, off);
    }
}
static void emit_store_base_off32(Buffer *b, int base, int reg, int off) {
    emit_rex_wrb(b, 0, reg, base);
    emit_byte(b, 0x89);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, base);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, base);
        emit_int32(b, off);
    }
}
static void emit_load_base_off32(Buffer *b, int dst, int base, int off) {
    emit_rex_wrb(b, 0, dst, base);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, dst, base);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, dst, base);
        emit_int32(b, off);
    }
}
static void emit_va_start(Buffer *b, const IRInst *inst, const RAResult *ra,
                          int gp_offset, int fp_offset, int overflow_off) {
    int ap = inst->call_args[0];
    ensure_reg(b, ap, REG_RAX, ra);
    int ap_reg = REG_RAX;
    emit_mov_imm(b, REG_RCX, gp_offset);
    emit_store_base_off32(b, ap_reg, REG_RCX, 0);
    emit_mov_imm(b, REG_RCX, fp_offset);
    emit_store_base_off32(b, ap_reg, REG_RCX, 4);
    emit_lea_rbp(b, REG_RCX, overflow_off);
    emit_store_base_off(b, ap_reg, REG_RCX, 8);
    emit_mov_rr(b, REG_RCX, REG_RSP);
    emit_store_base_off(b, ap_reg, REG_RCX, 16);
}
static void emit_load_gp_viabase(Buffer *b, int dst, int base, int index) {
    emit_add_rr(b, base, index);
    emit_load_base_off(b, dst, base, 0);
}
static void emit_va_arg(Buffer *b, const IRInst *inst, const RAResult *ra,
                        const RAResult *ra_xmm, int gp_spill_area) {
    int ap = inst->call_args[0];
    ensure_reg(b, ap, REG_RAX, ra);
    int ap_reg = REG_RAX;
    int dst = inst->dst;
    int is_float = inst->is_float;
    if (!is_float) {
        emit_load_base_off32(b, REG_RCX, ap_reg, 0);
        emit_cmp_imm32(b, REG_RCX, 48);
        size_t jae_ov = emit_jcc_rel32(b, 0x83);
        emit_load_base_off(b, REG_R11, ap_reg, 16);
        emit_load_gp_viabase(b, REG_RDX, REG_R11, REG_RCX);
        emit_load_base_off32(b, REG_RCX, ap_reg, 0);
        emit_add_imm32(b, REG_RCX, 8);
        emit_store_base_off32(b, ap_reg, REG_RCX, 0);
        size_t jmp_end = emit_jmp_rel32(b);
        size_t ov_off = b->len;
        patch_rel32(b, jae_ov, ov_off);
        emit_load_base_off(b, REG_R11, ap_reg, 8);
        emit_load_base_off(b, REG_RDX, REG_R11, 0);
        emit_add_imm32(b, REG_R11, 8);
        emit_store_base_off(b, ap_reg, REG_R11, 8);
        patch_rel32(b, jmp_end, b->len);
        if (dst >= 0) {
            if (ra && dst < ra->num_values && ra->reg[dst] >= 0
                && ra->reg[dst] < 16) {
                int dr = ra->reg[dst];
                if (dr != REG_RDX) emit_mov_rr(b, dr, REG_RDX);
            } else {
                spill_if_needed(b, dst, REG_RDX, ra);
            }
        }
    } else {
        emit_load_base_off32(b, REG_RCX, ap_reg, 4);
        emit_cmp_imm32(b, REG_RCX, 176);
        size_t jae_ov = emit_jcc_rel32(b, 0x83);
        emit_load_base_off(b, REG_R11, ap_reg, 16);
        emit_add_rr(b, REG_R11, REG_RCX);
        emit_sse_load_base_off(b, 0, REG_R11, 0);
        emit_load_base_off32(b, REG_RCX, ap_reg, 4);
        emit_add_imm32(b, REG_RCX, 16);
        emit_store_base_off32(b, ap_reg, REG_RCX, 4);
        size_t jmp_end = emit_jmp_rel32(b);
        size_t ov_off = b->len;
        patch_rel32(b, jae_ov, ov_off);
        emit_load_base_off(b, REG_R11, ap_reg, 8);
        emit_sse_load_base_off(b, 0, REG_R11, 0);
        emit_add_imm32(b, REG_R11, 8);
        emit_store_base_off(b, ap_reg, REG_R11, 8);
        patch_rel32(b, jmp_end, b->len);
        if (dst >= 0) {
            spill_if_needed_xmm(b, dst, 0, ra_xmm, gp_spill_area);
        }
    }
}
static void emit_epilogue(Buffer *b, int stack_size, const int cs_used[3]) {
    emit_add_rsp_imm32(b, stack_size);
    if (cs_used[2]) emit_pop_r(b, REG_R13);
    if (cs_used[1]) emit_pop_r(b, REG_R12);
    if (cs_used[0]) emit_pop_r(b, REG_RBX);
    emit_byte(b, 0x5D);
    emit_byte(b, 0xC3);
}
void codegen(const IRModule *ir, EmitModule *out) {
    for (size_t gi = 0; gi < ir->globals.len; gi++) {
        const IRGlobal *g = &ir->globals.data[gi];
        uint8_t binding = g->is_static ? 0 : 1 ;
        uint16_t shndx;
        size_t off;
        if (g->is_readonly) {
            shndx = 2;
            off = out->rodata.len;
            buffer_append(&out->rodata, g->init_bytes, g->size);
            while (out->rodata.len & 7) { char z = 0; buffer_append(&out->rodata, &z, 1); }
        } else if (g->init_bytes) {
            shndx = 3;
            off = out->data.len;
            buffer_append(&out->data, g->init_bytes, g->size);
            while (out->data.len & 7) { char z = 0; buffer_append(&out->data, &z, 1); }
        } else {
            shndx = 4;
            off = out->bss_size;
            out->bss_size += g->size;
            while (out->bss_size & 7) out->bss_size++;
        }
        emit_module_add_symbol(out, g->name, binding, 1 ,
                               shndx, off, g->size);
        for (int fi = 0; fi < g->num_fixups; fi++) {
            int tsym = emit_module_find_symbol(out, g->fixups[fi].sym);
            if (tsym < 0)
                tsym = emit_module_add_undefined(out, g->fixups[fi].sym);
            emit_module_add_data_reloc(out, off + g->fixups[fi].offset,
                                      10, tsym, 0);
        }
    }
    CallPatch *call_patches = ((void*)0);
    size_t num_call_patches = 0, cap_call_patches = 0;
    FnAddrPatch *fnaddr_patches = ((void*)0);
    size_t num_fnaddr_patches = 0, cap_fnaddr_patches = 0;
    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        const RAResult *ra = (const RAResult *)fn->ra;
        const RAResult *ra_xmm = (const RAResult *)fn->ra_xmm;
        size_t start_offset = out->text.len;
        int cs_used[3];
        collect_callee_saved(ra, cs_used);
        int cs_count = cs_used[0] + cs_used[1] + cs_used[2];
        curr_cs_count = cs_count;
        int cs_save_area = 8 * cs_count;
        int *alloca_off = xmalloc(fn->next_value_id * sizeof(int));
        for (int i = 0; i < fn->next_value_id; i++) alloca_off[i] = 0;
        int gp_spill_area = ra ? ra->stack_size : 0;
        int xmm_spill_area = ra_xmm ? ra_xmm->stack_size : 0;
        int pinned_area = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            if (inst->op == IR_ALLOCA && inst->alloca_bytes > 0 && inst->dst >= 0) {
                int bytes = inst->alloca_bytes;
                if (bytes % 8 != 0) bytes += 8 - (bytes % 8);
                pinned_area += bytes;
                alloca_off[inst->dst] = -(cs_save_area + gp_spill_area + xmm_spill_area + pinned_area);
            }
        }
        int *ld_off = xmalloc(fn->next_value_id * sizeof(int));
        for (int i = 0; i < fn->next_value_id; i++) ld_off[i] = 0;
        int ld_area = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            if (!value_is_ld(fn, inst->dst)) continue;
            if (inst->op == IR_ALLOCA && inst->alloca_bytes > 0) {
                ld_off[inst->dst] = alloca_off[inst->dst];
            } else if (inst->op != IR_PARAM) {
                ld_area += 16;
                ld_off[inst->dst] = -(cs_save_area + gp_spill_area + xmm_spill_area + pinned_area + ld_area);
            }
        }
        int stack_size = gp_spill_area + xmm_spill_area + pinned_area + ld_area;
        if (!ra && stack_size == 0) stack_size = 8 * fn->next_value_id;
        if (stack_size % 16 != 0) stack_size += 16 - (stack_size % 16);
        if (fn->is_variadic) stack_size += 176;
        if (stack_size % 16 != 0) stack_size += 16 - (stack_size % 16);
        curr_cs_count = cs_count;
        if (((cs_count) & 1) != 0) stack_size += 8;
        emit_byte(&out->text, 0x55);
        emit_rex_w(&out->text);
        emit_byte(&out->text, 0x89);
        emit_byte(&out->text, 0xE5);
        if (cs_used[0]) emit_push_r(&out->text, REG_RBX);
        if (cs_used[1]) emit_push_r(&out->text, REG_R12);
        if (cs_used[2]) emit_push_r(&out->text, REG_R13);
        int nparams = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            if (fn->insts.data[j].op == IR_PARAM) nparams++;
            else break;
        }
        int arrive_reg[64];
        int arrive_is_xmm[64];
        int stack_off[64];
        int gp_reg_idx = 0, xmm_reg_idx = 0, stack_arg_idx = 0;
        for (int p = 0; p < nparams; p++) {
            const IRInst *pi = &fn->insts.data[p];
            int is_ld = (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                         value_is_ld(fn, pi->dst));
            int is_float = !is_ld && (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                            value_is_float_class(fn, pi->dst));
            if (is_ld) {
                arrive_reg[p] = -1;
                arrive_is_xmm[p] = 0;
                stack_off[p] = 16 + 8 * stack_arg_idx;
                stack_arg_idx += 2;
                ld_off[pi->dst] = stack_off[p];
            } else if (is_float) {
                if (xmm_reg_idx < 8) {
                    arrive_reg[p] = xmm_reg_idx;
                    arrive_is_xmm[p] = 1;
                    xmm_reg_idx++;
                } else {
                    arrive_reg[p] = -1;
                    arrive_is_xmm[p] = 0;
                    stack_off[p] = 16 + 8 * stack_arg_idx++;
                }
            } else {
                if (gp_reg_idx < 6) {
                    arrive_reg[p] = SYSV_ARG_REGS[gp_reg_idx++];
                    arrive_is_xmm[p] = 0;
                } else {
                    arrive_reg[p] = -1;
                    arrive_is_xmm[p] = 0;
                    stack_off[p] = 16 + 8 * stack_arg_idx++;
                }
            }
        }
        int va_gp_offset = 0, va_fp_offset = 0, va_overflow_off = 16;
        if (fn->is_variadic) {
            va_gp_offset = 8 * gp_reg_idx;
            va_fp_offset = 48 + 16 * xmm_reg_idx;
            va_overflow_off = 16 + 8 * stack_arg_idx;
        }
        emit_sub_rsp_imm32(&out->text, stack_size);
        if (fn->is_variadic) {
            emit_store_rsp_off(&out->text, REG_RDI, 0);
            emit_store_rsp_off(&out->text, REG_RSI, 8);
            emit_store_rsp_off(&out->text, REG_RDX, 16);
            emit_store_rsp_off(&out->text, REG_RCX, 24);
            emit_store_rsp_off(&out->text, REG_R8, 32);
            emit_store_rsp_off(&out->text, REG_R9, 40);
            emit_sse_store_rsp_off(&out->text, 0, 48);
            emit_sse_store_rsp_off(&out->text, 1, 64);
            emit_sse_store_rsp_off(&out->text, 2, 80);
            emit_sse_store_rsp_off(&out->text, 3, 96);
            emit_sse_store_rsp_off(&out->text, 4, 112);
            emit_sse_store_rsp_off(&out->text, 5, 128);
            emit_sse_store_rsp_off(&out->text, 6, 144);
            emit_sse_store_rsp_off(&out->text, 7, 160);
        }
        for (int p = nparams - 1; p >= 0; p--) {
            if (arrive_reg[p] < 0) continue;
            if (arrive_is_xmm[p]) {
                emit_sub_rsp_imm32(&out->text, 8);
                emit_sse_store_rsp(&out->text, arrive_reg[p]);
            } else {
                emit_push_r(&out->text, arrive_reg[p]);
            }
        }
        for (int p = 0; p < nparams; p++) {
            const IRInst *pi = &fn->insts.data[p];
            int is_ld = (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                         value_is_ld(fn, pi->dst));
            int is_float = (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                            value_is_float_class(fn, pi->dst));
            if (arrive_reg[p] < 0) {
                if (is_ld) {
                    continue;
                }
                if (is_float) {
                    int pdr_xmm = (ra_xmm && pi->dst >= 0 &&
                                   pi->dst < ra_xmm->num_values)
                                  ? ra_xmm->reg[pi->dst] : -1;
                    if (pdr_xmm >= 0) {
                        emit_sse_load_spill(&out->text, pdr_xmm, stack_off[p]);
                    } else {
                        emit_sse_load_spill(&out->text, 14, stack_off[p]);
                        spill_if_needed_xmm(&out->text, pi->dst, 14,
                                             ra_xmm, gp_spill_area);
                    }
                } else {
                    int pdr = (ra && pi->dst >= 0 && pi->dst < ra->num_values)
                              ? ra->reg[pi->dst] : -1;
                    if (pdr >= 0) {
                        emit_load_spill(&out->text, pdr, stack_off[p]);
                    } else {
                        emit_load_spill(&out->text, REG_RAX, stack_off[p]);
                        spill_if_needed(&out->text, pi->dst, REG_RAX, ra);
                    }
                }
                continue;
            }
            if (is_float) {
                int pdr_xmm = (ra_xmm && pi->dst >= 0 &&
                               pi->dst < ra_xmm->num_values)
                              ? ra_xmm->reg[pi->dst] : -1;
                if (pdr_xmm >= 0) {
                    emit_sse_load_rsp(&out->text, pdr_xmm);
                } else {
                    emit_sse_load_rsp(&out->text, 14);
                    spill_if_needed_xmm(&out->text, pi->dst, 14,
                                         ra_xmm, gp_spill_area);
                }
                emit_add_rsp_imm32(&out->text, 8);
            } else {
                int pdr = (ra && pi->dst >= 0 && pi->dst < ra->num_values)
                          ? ra->reg[pi->dst] : -1;
                if (pdr >= 0) {
                    emit_pop_r(&out->text, pdr);
                } else {
                    emit_pop_r(&out->text, REG_RAX);
                    spill_if_needed(&out->text, pi->dst, REG_RAX, ra);
                }
            }
        }
        int nlabels = fn->next_label_id;
        size_t *label_off = ((void*)0);
        if (nlabels > 0) {
            label_off = xmalloc(nlabels * sizeof(size_t));
            for (int L = 0; L < nlabels; L++) label_off[L] = (size_t)-1;
        }
        Patch *patches = ((void*)0);
        size_t num_patches = 0, cap_patches = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            int dr;
            if (value_is_float_class(fn, inst->dst)) {
                dr = (ra_xmm && inst->dst >= 0 && inst->dst < ra_xmm->num_values)
                     ? ra_xmm->reg[inst->dst] : -1;
            } else {
                dr = (ra && inst->dst >= 0 && inst->dst < ra->num_values)
                     ? ra->reg[inst->dst] : -1;
            }
            switch (inst->op) {
            case IR_CONST: {
                if (value_is_ld(fn, inst->dst)) {
                    size_t patch = emit_lea_rip(&out->text, REG_RCX);
                    int gsym = emit_module_find_symbol(out, inst->call_name);
                    if (gsym < 0)
                        gsym = emit_module_add_undefined(out, inst->call_name);
                    emit_module_add_reloc(out, patch, 2, gsym, -4);
                    emit_x87_fldtRCX(&out->text);
                    emit_ld_store(&out->text, inst->dst, ld_off);
                } else if (inst->is_float) {
                    if (dr >= 0) {
                        emit_float_const(&out->text, dr, inst->float_imm);
                    } else {
                        emit_float_const(&out->text, 14, inst->float_imm);
                        spill_if_needed_xmm(&out->text, inst->dst, 14,
                                            ra_xmm, gp_spill_area);
                    }
                } else if (dr >= 0) {
                    emit_mov_imm(&out->text, dr, inst->imm);
                } else {
                    emit_mov_imm(&out->text, REG_RAX, inst->imm);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }
            case IR_ADD: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                emit_add_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_SUB: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                emit_sub_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_MUL: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                emit_imul_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_DIV:
            case IR_MOD: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                if (inst->is_unsigned) {
                    emit_xor_rr(&out->text, REG_RDX);
                    emit_rex_w(&out->text);
                    emit_byte(&out->text, 0xF7);
                    emit_byte(&out->text, 0xF1);
                } else {
                    emit_cqto(&out->text);
                    emit_idiv_rcx(&out->text);
                }
                if (inst->op == IR_DIV) {
                    mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                    if (dr >= 0 && dr != REG_RAX)
                        emit_mov_rr(&out->text, dr, REG_RAX);
                    spill_if_needed(&out->text, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                } else {
                    mask_to_width(&out->text, REG_RDX, inst->width, inst->is_unsigned);
                    if (dr >= 0)
                        emit_mov_rr(&out->text, dr, REG_RDX);
                    spill_if_needed(&out->text, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                }
                break;
            }
            case IR_NEG: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                emit_neg_r(&out->text, REG_RAX);
                mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_BNOT: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                emit_not_r(&out->text, REG_RAX);
                mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_BAND:
            case IR_BOR:
            case IR_BXOR: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                if (inst->op == IR_BAND)
                    emit_and_rr(&out->text, REG_RAX, REG_RCX);
                else if (inst->op == IR_BOR)
                    emit_or_rr(&out->text, REG_RAX, REG_RCX);
                else
                    emit_bitxor_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_SHL:
            case IR_SHR: {
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                if (inst->op == IR_SHL)
                    emit_shl_rcx(&out->text, REG_RAX);
                else if (inst->is_unsigned)
                    emit_shr_rcx(&out->text, REG_RAX);
                else
                    emit_sar_rcx(&out->text, REG_RAX);
                mask_to_width(&out->text, REG_RAX, inst->width, inst->is_unsigned);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_EQ:
            case IR_NE:
            case IR_LT:
            case IR_LE:
            case IR_GT:
            case IR_GE: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                uint8_t cc = ir_cmp_to_setcc(inst->op, inst->is_unsigned);
                emit_cmp_produce(&out->text, REG_RAX, REG_RCX, REG_RDX, cc);
                if (dr >= 0) {
                    if (dr != REG_RDX) emit_mov_rr(&out->text, dr, REG_RDX);
                } else {
                    spill_if_needed(&out->text, inst->dst, REG_RDX, ra);
                }
                break;
            }
            case IR_ALLOCA:
                break;
            case IR_COPY: {
                if (ra && inst->a >= 0 && inst->a < ra->num_values &&
                    inst->dst >= 0 && inst->dst < ra->num_values &&
                    ra->reg[inst->dst] == ra->reg[inst->a] &&
                    ra->reg[inst->dst] >= 0) {
                    break;
                }
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                if (dr >= 0) {
                    if (dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                } else {
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }
            case IR_SEXT:
            case IR_ZEXT: {
                int src_w = inst->imm;
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                if (inst->op == IR_SEXT)
                    emit_movsx_rr(&out->text, REG_RAX, REG_RAX, src_w);
                else
                    emit_movzx_rr(&out->text, REG_RAX, REG_RAX, src_w);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_TRUNC: {
                if (ra && inst->a >= 0 && inst->a < ra->num_values &&
                    inst->dst >= 0 && inst->dst < ra->num_values &&
                    ra->reg[inst->dst] == ra->reg[inst->a] &&
                    ra->reg[inst->dst] >= 0) {
                    mask_to_width(&out->text, ra->reg[inst->dst],
                                  inst->width, inst->is_unsigned);
                    break;
                }
                if (dr >= 0) {
                    ensure_reg(&out->text, inst->a, dr, ra);
                    mask_to_width(&out->text, dr, inst->width,
                                  inst->is_unsigned);
                } else {
                    ensure_reg(&out->text, inst->a, REG_RAX, ra);
                    mask_to_width(&out->text, REG_RAX, inst->width,
                                  inst->is_unsigned);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }
            case IR_LOAD: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                if (dr >= 0) {
                    if (dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                } else {
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }
            case IR_STORE: {
                int ar = (ra && inst->a >= 0 && inst->a < ra->num_values)
                         ? ra->reg[inst->a] : -1;
                if (ar >= 0) {
                    ensure_reg(&out->text, inst->b, ar, ra);
                } else {
                    ensure_reg(&out->text, inst->b, REG_RAX, ra);
                    spill_if_needed(&out->text, inst->a, REG_RAX, ra);
                }
                break;
            }
            case IR_ADDR: {
                int off = 0;
                if (inst->a >= 0 && inst->a < fn->next_value_id)
                    off = alloca_off[inst->a];
                if (off == 0) {
                    fprintf(stderr, "fakecc: IR_ADDR on non-pinned alloca %d\n", inst->a);
                    exit(1);
                }
                if (dr >= 0) {
                    emit_lea_rbp(&out->text, dr, off);
                } else {
                    emit_lea_rbp(&out->text, REG_RAX, off);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }
            case IR_GADDR: {
                int target = dr >= 0 ? dr : REG_RAX;
                int gsym = emit_module_find_symbol(out, inst->call_name);
                int defined_here = (gsym >= 0 && out->syms[gsym].shndx != 0);
                if (defined_here) {
                    size_t patch = emit_lea_rip(&out->text, target);
                    emit_module_add_reloc(out, patch, 2, gsym, -4);
                } else {
                    if (gsym < 0)
                        gsym = emit_module_add_undefined(out, inst->call_name);
                    size_t patch = emit_load_rip(&out->text, target);
                    emit_module_add_reloc(out, patch, 9, gsym, -4);
                }
                if (dr < 0)
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                break;
            }
            case IR_FADDR: {
                int target = dr >= 0 ? dr : REG_RAX;
                size_t patch = emit_lea_rip(&out->text, target);
                if (num_fnaddr_patches >= cap_fnaddr_patches) {
                    cap_fnaddr_patches = cap_fnaddr_patches ? cap_fnaddr_patches * 2 : 8;
                    fnaddr_patches = xrealloc(fnaddr_patches,
                                               cap_fnaddr_patches * sizeof(FnAddrPatch));
                }
                fnaddr_patches[num_fnaddr_patches].patch_off = patch;
                fnaddr_patches[num_fnaddr_patches].fn_name = xstrdup(inst->call_name);
                num_fnaddr_patches++;
                if (dr < 0)
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                break;
            }
            case IR_LOAD_PTR: {
                ensure_reg(&out->text, inst->a, REG_RCX, ra);
                if (value_is_ld(fn, inst->dst)) {
                    emit_byte(&out->text, 0xDB);
                    emit_modrm(&out->text, 0, 5, REG_RCX & 7);
                    emit_ld_store(&out->text, inst->dst, ld_off);
                } else if (value_is_float_class(fn, inst->dst)) {
                    int is_float = (inst->width == 4);
                    emit_sse_load_via_ptr(&out->text,
                                          dr >= 0 ? dr : 14,
                                          REG_RCX, is_float);
                    if (dr < 0)
                        spill_if_needed_xmm(&out->text, inst->dst, 14,
                                            ra_xmm, gp_spill_area);
                } else {
                    emit_load_via_ptr(&out->text,
                                      dr >= 0 ? dr : REG_RAX,
                                      REG_RCX, inst->width, inst->is_unsigned);
                    if (dr < 0)
                        spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }
            case IR_STORE_PTR: {
                ensure_reg(&out->text, inst->a, REG_RCX, ra);
                if (value_is_ld(fn, inst->b)) {
                    emit_ld_load(&out->text, inst->b, ld_off);
                    ensure_reg(&out->text, inst->a, REG_RCX, ra);
                    emit_byte(&out->text, 0xDB);
                    emit_modrm(&out->text, 0, 7, REG_RCX & 7);
                } else if (value_is_float_class(fn, inst->b)) {
                    int is_float = (inst->width == 4);
                    ensure_reg_xmm(&out->text, inst->b, 14, ra_xmm,
                                   gp_spill_area);
                    emit_sse_store_via_ptr(&out->text, REG_RCX, 14,
                                           is_float);
                } else {
                    ensure_reg(&out->text, inst->b, REG_RAX, ra);
                    emit_store_via_ptr(&out->text, REG_RCX, REG_RAX, inst->width);
                }
                break;
            }
            case IR_LABEL: {
                if (inst->imm >= 0 && inst->imm < nlabels) {
                    label_off[inst->imm] = out->text.len;
                }
                break;
            }
            case IR_BR: {
                size_t patch = emit_jmp_rel32(&out->text);
                size_t after = out->text.len;
                do { if (num_patches >= cap_patches) { cap_patches = cap_patches ? cap_patches * 2 : 8; patches = xrealloc(patches, cap_patches * sizeof(Patch)); } patches[num_patches].patch_off = (patch); patches[num_patches].label = (inst->imm); patches[num_patches].after_off = (after); num_patches++; } while (0);
                break;
            }
            case IR_CBR: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                emit_test_rr(&out->text, REG_RAX);
                size_t p1 = emit_jcc_rel32(&out->text, 0x85);
                size_t a1 = out->text.len;
                do { if (num_patches >= cap_patches) { cap_patches = cap_patches ? cap_patches * 2 : 8; patches = xrealloc(patches, cap_patches * sizeof(Patch)); } patches[num_patches].patch_off = (p1); patches[num_patches].label = (inst->imm); patches[num_patches].after_off = (a1); num_patches++; } while (0);
                size_t p2 = emit_jmp_rel32(&out->text);
                size_t a2 = out->text.len;
                do { if (num_patches >= cap_patches) { cap_patches = cap_patches ? cap_patches * 2 : 8; patches = xrealloc(patches, cap_patches * sizeof(Patch)); } patches[num_patches].patch_off = (p2); patches[num_patches].label = (inst->b); patches[num_patches].after_off = (a2); num_patches++; } while (0);
                break;
            }
            case IR_PARAM:
                break;
            case IR_CALL: {
                if (inst->call_name && strcmp(inst->call_name, "__syscall") == 0) {
                    static const int SYS_ARG_REGS[7] = {
                        REG_RAX, REG_RDI, REG_RSI, REG_RDX,
                        REG_R10, REG_R8, REG_R9
                    };
                    int nargs = inst->call_nargs;
                    for (int k = 0; k < nargs; k++) {
                        ensure_reg(&out->text, inst->call_args[k], REG_RCX, ra);
                        emit_push_r(&out->text, REG_RCX);
                    }
                    for (int k = nargs - 1; k >= 0; k--) {
                        emit_pop_r(&out->text, SYS_ARG_REGS[k]);
                    }
                    emit_byte(&out->text, 0x0F);
                    emit_byte(&out->text, 0x05);
                    if (dr >= 0) {
                        if (dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                    } else {
                        spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                    }
                    break;
                }
                if (inst->call_name
                    && strcmp(inst->call_name, "__builtin_ctzll") == 0) {
                    ensure_reg(&out->text, inst->call_args[0], REG_RCX, ra);
                    emit_rex_wrb(&out->text, 1, REG_RAX, REG_RCX);
                    emit_byte(&out->text, 0x0F);
                    emit_byte(&out->text, 0xBC);
                    emit_modrm(&out->text, 3, REG_RAX & 7, REG_RCX & 7);
                    if (inst->dst >= 0) {
                        if (dr >= 0) {
                            if (dr != REG_RAX)
                                emit_mov_rr(&out->text, dr, REG_RAX);
                        } else {
                            spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                        }
                    }
                    break;
                }
                if (inst->call_name && strcmp(inst->call_name, "va_start") == 0) {
                    emit_va_start(&out->text, inst, ra, va_gp_offset,
                                  va_fp_offset, va_overflow_off);
                    break;
                }
                if (inst->call_name && strcmp(inst->call_name, "va_end") == 0) {
                    break;
                }
                if (inst->call_name && strcmp(inst->call_name, "va_arg") == 0) {
                    emit_va_arg(&out->text, inst, ra, ra_xmm, gp_spill_area);
                    break;
                }
                int nargs = inst->call_nargs;
                int target_reg[16];
                int target_is_xmm[16];
                int n_gp = 0, n_xmm = 0, n_stack = 0;
                for (int k = 0; k < nargs; k++) {
                    int is_ld = value_is_ld(fn, inst->call_args[k]);
                    int is_float = !is_ld && value_is_float_class(fn, inst->call_args[k]);
                    if (is_ld) {
                        target_reg[k] = -1;
                        target_is_xmm[k] = 0;
                        n_stack += 2;
                    } else if (is_float) {
                        if (n_xmm < 8) {
                            target_reg[k] = n_xmm;
                            target_is_xmm[k] = 1;
                            n_xmm++;
                        } else {
                            target_reg[k] = -1;
                            target_is_xmm[k] = 0;
                            n_stack++;
                        }
                    } else {
                        if (n_gp < 6) {
                            target_reg[k] = SYSV_ARG_REGS[n_gp++];
                            target_is_xmm[k] = 0;
                        } else {
                            target_reg[k] = -1;
                            target_is_xmm[k] = 0;
                            n_stack++;
                        }
                    }
                }
                int need_pad = (n_stack & 1);
                if (need_pad) emit_sub_rsp_imm32(&out->text, 8);
                if (!inst->call_name)
                    ensure_reg(&out->text, inst->call_callee, REG_R11, ra);
                for (int k = nargs - 1; k >= 0; k--) {
                    if (target_reg[k] >= 0) continue;
                    if (value_is_ld(fn, inst->call_args[k])) {
                        emit_ld_load(&out->text, inst->call_args[k], ld_off);
                        emit_sub_rsp_imm32(&out->text, 16);
                        emit_x87_fstpt_rsp(&out->text);
                    } else if (target_is_xmm[k]) {
                        ensure_reg_xmm(&out->text, inst->call_args[k],
                                       14, ra_xmm, gp_spill_area);
                        emit_sub_rsp_imm32(&out->text, 8);
                        emit_sse_store_rsp(&out->text, 14);
                    } else {
                        ensure_reg(&out->text, inst->call_args[k], REG_RAX, ra);
                        emit_push_r(&out->text, REG_RAX);
                    }
                }
                for (int k = nargs - 1; k >= 0; k--) {
                    if (target_reg[k] < 0) continue;
                    if (target_is_xmm[k]) {
                        ensure_reg_xmm(&out->text, inst->call_args[k],
                                       14, ra_xmm, gp_spill_area);
                        emit_sub_rsp_imm32(&out->text, 8);
                        emit_sse_store_rsp(&out->text, 14);
                    } else {
                        ensure_reg(&out->text, inst->call_args[k], REG_RAX, ra);
                        emit_push_r(&out->text, REG_RAX);
                    }
                }
                for (int k = 0; k < nargs; k++) {
                    if (target_reg[k] < 0) continue;
                    if (target_is_xmm[k]) {
                        emit_sse_load_rsp(&out->text, target_reg[k]);
                        emit_add_rsp_imm32(&out->text, 8);
                    } else {
                        emit_pop_r(&out->text, target_reg[k]);
                    }
                }
                emit_byte(&out->text, 0xB0 | REG_RAX);
                emit_byte(&out->text, (uint8_t)n_xmm);
                if (inst->call_name) {
                    size_t poff = emit_call_rel32(&out->text);
                    size_t aft = out->text.len;
                    if (num_call_patches >= cap_call_patches) {
                        cap_call_patches = cap_call_patches ? cap_call_patches * 2 : 8;
                        call_patches = xrealloc(call_patches,
                                                 cap_call_patches * sizeof(CallPatch));
                    }
                    call_patches[num_call_patches].patch_off = poff;
                    call_patches[num_call_patches].callee = xstrdup(inst->call_name);
                    call_patches[num_call_patches].after_off = aft;
                    num_call_patches++;
                } else {
                    emit_indirect_call(&out->text, REG_R11);
                }
                int cleanup = n_stack * 8 + (need_pad ? 8 : 0);
                if (cleanup > 0) emit_add_rsp_imm32(&out->text, cleanup);
                if (value_is_ld(fn, inst->dst)) {
                    if (inst->dst >= 0)
                        emit_ld_store(&out->text, inst->dst, ld_off);
                } else if (value_is_float_class(fn, inst->dst)) {
                    if (inst->dst >= 0) {
                        if (dr >= 0 && dr != 0)
                            emit_sse_mov_rr(&out->text, dr, 0);
                        else if (dr < 0)
                            spill_if_needed_xmm(&out->text, inst->dst, 0,
                                                ra_xmm, gp_spill_area);
                    }
                } else {
                    if (inst->dst >= 0) {
                        if (dr >= 0) {
                            if (dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                        } else {
                            spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                        }
                    }
                }
                break;
            }
            case IR_RETURN: {
                if (inst->a != -1) {
                    if (value_is_ld(fn, inst->a))
                        emit_ld_load(&out->text, inst->a, ld_off);
                    else if (value_is_float_class(fn, inst->a))
                        ensure_reg_xmm(&out->text, inst->a, 0, ra_xmm,
                                       gp_spill_area);
                    else
                        ensure_reg(&out->text, inst->a, REG_RAX, ra);
                }
                emit_epilogue(&out->text, stack_size, cs_used);
                break;
            }
            case IR_FADD:
            case IR_FSUB:
            case IR_FMUL:
            case IR_FDIV: {
                if (value_is_ld(fn, inst->dst)) {
                    emit_ld_load(&out->text, inst->a, ld_off);
                    emit_ld_load(&out->text, inst->b, ld_off);
                    int op;
                    switch (inst->op) {
                    case IR_FADD: op = 0xC1; break;
                    case IR_FSUB: op = 0xE9; break;
                    case IR_FMUL: op = 0xC9; break;
                    default: op = 0xF9; break;
                    }
                    emit_x87_arith_pop(&out->text, op);
                    emit_ld_store(&out->text, inst->dst, ld_off);
                    break;
                }
                int is_float = (inst->width == 4);
                ensure_reg_xmm(&out->text, inst->a, 14, ra_xmm,
                               gp_spill_area);
                ensure_reg_xmm(&out->text, inst->b, 15, ra_xmm,
                               gp_spill_area);
                int op;
                switch (inst->op) {
                case IR_FADD: op = 0x58; break;
                case IR_FSUB: op = 0x5C; break;
                case IR_FMUL: op = 0x59; break;
                default: op = 0x5E; break;
                }
                emit_sse_arith(&out->text, op, 14, 15,
                               is_float);
                if (dr >= 0 && dr != 14)
                    emit_sse_mov_rr(&out->text, dr, 14);
                spill_if_needed_xmm(&out->text, inst->dst, 14,
                                    ra_xmm, gp_spill_area);
                break;
            }
            case IR_FCMP: {
                if (value_is_ld(fn, inst->a) || value_is_ld(fn, inst->b)) {
                    int is_lt_le = (inst->is_unsigned == 0 || inst->is_unsigned == 1);
                    emit_xor_rr(&out->text, REG_RDX);
                    if (is_lt_le) {
                        emit_ld_load(&out->text, inst->a, ld_off);
                        emit_ld_load(&out->text, inst->b, ld_off);
                    } else {
                        emit_ld_load(&out->text, inst->b, ld_off);
                        emit_ld_load(&out->text, inst->a, ld_off);
                    }
                    emit_x87_fcomip(&out->text);
                    emit_byte(&out->text, 0xDD); emit_byte(&out->text, 0xD8);
                    uint8_t cc;
                    switch (inst->is_unsigned) {
                    case 0: cc = 0x97; break;
                    case 1: cc = 0x93; break;
                    case 2: cc = 0x97; break;
                    case 3: cc = 0x93; break;
                    case 4: cc = 0x94; break;
                    default: cc = 0x95; break;
                    }
                    emit_setcc_r(&out->text, cc, REG_RDX);
                    int dr_gp = (ra && inst->dst >= 0 && inst->dst < ra->num_values)
                                ? ra->reg[inst->dst] : -1;
                    if (dr_gp >= 0 && dr_gp != REG_RDX)
                        emit_mov_rr(&out->text, dr_gp, REG_RDX);
                    spill_if_needed(&out->text, inst->dst,
                                    dr_gp >= 0 ? dr_gp : REG_RDX, ra);
                    break;
                }
                int is_float = (inst->width == 4);
                ensure_reg_xmm(&out->text, inst->a, 14, ra_xmm,
                               gp_spill_area);
                ensure_reg_xmm(&out->text, inst->b, 15, ra_xmm,
                               gp_spill_area);
                uint8_t cc;
                switch (inst->is_unsigned) {
                case 0: cc = 0x92; break;
                case 1: cc = 0x96; break;
                case 2: cc = 0x97; break;
                case 3: cc = 0x93; break;
                case 4: cc = 0x94; break;
                default: cc = 0x95; break;
                }
                emit_xor_rr(&out->text, REG_RDX);
                emit_sse_ucomi(&out->text, 14, 15,
                               is_float);
                emit_setcc_r(&out->text, cc, REG_RDX);
                if (dr >= 0 && dr != REG_RDX)
                    emit_mov_rr(&out->text, dr, REG_RDX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RDX, ra);
                break;
            }
            case IR_SITOFP: {
                if (value_is_ld(fn, inst->dst)) {
                    int pdr = (ra && inst->a >= 0 && inst->a < ra->num_values)
                              ? ra->reg[inst->a] : -1;
                    int src = (pdr >= 0) ? pdr : REG_RAX;
                    if (pdr < 0) emit_load_spill(&out->text, REG_RAX, spill_offset(ra->spill_slot[inst->a]));
                    emit_ld_from_gp_int(&out->text, src, inst->dst, ld_off);
                    break;
                }
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                int is_64 = (inst->width == 8);
                emit_sse_cvtsi2sd(&out->text, 14, REG_RAX, is_64);
                if (inst->width == 4)
                    emit_sse_cvtsd2ss(&out->text, 14, 14);
                if (dr >= 0 && dr != 14)
                    emit_sse_mov_rr(&out->text, dr, 14);
                spill_if_needed_xmm(&out->text, inst->dst, 14,
                                    ra_xmm, gp_spill_area);
                break;
            }
            case IR_FPTOSI: {
                if (value_is_ld(fn, inst->a)) {
                    emit_ld_load(&out->text, inst->a, ld_off);
                    emit_sub_rsp_imm32(&out->text, 8);
                    emit_byte(&out->text, 0x48); emit_byte(&out->text, 0x8D);
                    emit_modrm(&out->text, 0, REG_RCX & 7, 4);
                    emit_byte(&out->text, 0x24);
                    emit_byte(&out->text, 0xDB);
                    emit_modrm(&out->text, 0, 1, REG_RCX & 7);
                    int dr_gp = (ra && inst->dst >= 0 && inst->dst < ra->num_values)
                                ? ra->reg[inst->dst] : -1;
                    int dst_reg = (dr_gp >= 0) ? dr_gp : REG_RAX;
                    emit_byte(&out->text, 0x48); emit_byte(&out->text, 0x8B);
                    emit_modrm(&out->text, 0, dst_reg & 7, 4);
                    emit_byte(&out->text, 0x24);
                    emit_add_rsp_imm32(&out->text, 8);
                    if (dr_gp >= 0 && dr_gp != dst_reg)
                        emit_mov_rr(&out->text, dr_gp, dst_reg);
                    spill_if_needed(&out->text, inst->dst,
                                    dr_gp >= 0 ? dr_gp : REG_RAX, ra);
                    break;
                }
                ensure_reg_xmm(&out->text, inst->a, 14, ra_xmm,
                               gp_spill_area);
                int src_float = (inst->imm == 4);
                int dst_64 = (inst->width == 8);
                if (src_float)
                    emit_sse_cvtss2si(&out->text, REG_RAX, 14, dst_64);
                else
                    emit_sse_cvtsd2si(&out->text, REG_RAX, 14, dst_64);
                if (dr >= 0 && dr != REG_RAX)
                    emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }
            case IR_FPEXT: {
                if (value_is_ld(fn, inst->dst)) {
                    ensure_reg_xmm(&out->text, inst->a, 14, ra_xmm,
                                   gp_spill_area);
                    emit_ld_addr(&out->text, REG_RCX, ld_off[inst->dst]);
                    emit_sse_store_via_ptr(&out->text, REG_RCX, 14, 0);
                    emit_byte(&out->text, 0xDD);
                    emit_modrm(&out->text, 0, 0, REG_RCX & 7);
                    emit_x87_fstptRCX(&out->text);
                    break;
                }
                ensure_reg_xmm(&out->text, inst->a, 14, ra_xmm,
                               gp_spill_area);
                emit_sse_cvtss2sd(&out->text, 14, 14);
                if (dr >= 0 && dr != 14)
                    emit_sse_mov_rr(&out->text, dr, 14);
                spill_if_needed_xmm(&out->text, inst->dst, 14,
                                    ra_xmm, gp_spill_area);
                break;
            }
            case IR_FPTRUNC: {
                if (value_is_ld(fn, inst->a)) {
                    emit_ld_load(&out->text, inst->a, ld_off);
                    emit_ld_addr(&out->text, REG_RCX, ld_off[inst->a]);
                    emit_byte(&out->text, 0xDD);
                    emit_modrm(&out->text, 0, 3, REG_RCX & 7);
                    emit_sse_load_via_ptr(&out->text, 14, REG_RCX, 0);
                    if (dr >= 0 && dr != 14)
                        emit_sse_mov_rr(&out->text, dr, 14);
                    spill_if_needed_xmm(&out->text, inst->dst, 14,
                                        ra_xmm, gp_spill_area);
                    break;
                }
                ensure_reg_xmm(&out->text, inst->a, 14, ra_xmm,
                               gp_spill_area);
                emit_sse_cvtsd2ss(&out->text, 14, 14);
                if (dr >= 0 && dr != 14)
                    emit_sse_mov_rr(&out->text, dr, 14);
                spill_if_needed_xmm(&out->text, inst->dst, 14,
                                    ra_xmm, gp_spill_area);
                break;
            }
            }
        }
        int needs_ret = 0;
        if (fn->insts.len == 0) {
            needs_ret = 1;
        } else {
            IROpcode last = fn->insts.data[fn->insts.len - 1].op;
            if (last != IR_RETURN && last != IR_BR && last != IR_CBR)
                needs_ret = 1;
        }
        if (needs_ret)
            emit_epilogue(&out->text, stack_size, cs_used);
        for (size_t pi = 0; pi < num_patches; pi++) {
            Patch *p = &patches[pi];
            if (p->label < 0 || p->label >= nlabels ||
                label_off[p->label] == (size_t)-1) {
                fprintf(stderr, "fakecc: unresolved label %d in codegen\n", p->label);
                exit(1);
            }
            int64_t rel = (int64_t)label_off[p->label] - (int64_t)p->after_off;
            if (rel < (-2147483647-1) || rel > 2147483647) {
                fprintf(stderr, "fakecc: branch displacement out of range\n");
                exit(1);
            }
            int32_t rel32 = (int32_t)rel;
            memcpy(out->text.data + p->patch_off, &rel32, 4);
        }
        free(patches);
        free(label_off);
        free(alloca_off);
        free(ld_off);
        size_t fn_size = out->text.len - start_offset;
        uint8_t binding = fn->is_static ? 0 : 1 ;
        emit_module_add_symbol(out, fn->name, binding, 2 ,
                               (uint16_t)1, start_offset, fn_size);
    }
    for (size_t pi = 0; pi < num_call_patches; pi++) {
        CallPatch *cp = &call_patches[pi];
        size_t target = (size_t)-1;
        for (size_t si = 0; si < out->num_syms; si++) {
            if (out->syms[si].name && strcmp(out->syms[si].name, cp->callee) == 0
                && out->syms[si].shndx != 0) {
                target = out->syms[si].value;
                break;
            }
        }
        if (target == (size_t)-1) {
            int esym = emit_module_find_symbol(out, cp->callee);
            if (esym < 0)
                esym = emit_module_add_undefined(out, cp->callee);
            emit_module_add_reloc(out, cp->patch_off, 4, esym, -4);
            free(cp->callee);
            continue;
        }
        int64_t rel = (int64_t)target - (int64_t)cp->after_off;
        if (rel < (-2147483647-1) || rel > 2147483647) {
            fprintf(stderr, "fakecc: call displacement out of range\n");
            exit(1);
        }
        int32_t rel32 = (int32_t)rel;
        memcpy(out->text.data + cp->patch_off, &rel32, 4);
        free(cp->callee);
    }
    free(call_patches);
    for (size_t pi = 0; pi < num_fnaddr_patches; pi++) {
        FnAddrPatch *fp = &fnaddr_patches[pi];
        size_t target = (size_t)-1;
        for (size_t si = 0; si < out->num_syms; si++) {
            if (out->syms[si].name && strcmp(out->syms[si].name, fp->fn_name) == 0
                && out->syms[si].shndx != 0) {
                target = out->syms[si].value;
                break;
            }
        }
        if (target == (size_t)-1) {
            int esym = emit_module_find_symbol(out, fp->fn_name);
            if (esym < 0)
                esym = emit_module_add_undefined(out, fp->fn_name);
            emit_module_add_reloc(out, fp->patch_off, 2, esym, -4);
            free(fp->fn_name);
            continue;
        }
        int64_t rel = (int64_t)target - (int64_t)(fp->patch_off + 4);
        if (rel < (-2147483647-1) || rel > 2147483647) {
            fprintf(stderr, "fakecc: function address displacement out of range\n");
            exit(1);
        }
        int32_t rel32 = (int32_t)rel;
        memcpy(out->text.data + fp->patch_off, &rel32, 4);
        free(fp->fn_name);
    }
    free(fnaddr_patches);
}
