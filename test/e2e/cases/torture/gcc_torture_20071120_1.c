// expect: 0
package main;

extern void* memcpy(void*, const void*, unsigned long);
extern void* memset(void*, int, unsigned long);
extern int memcmp(const void*, const void*, unsigned long);
extern void* memmove(void*, const void*, unsigned long);
extern int strcmp(const char*, const char*);
extern int strncmp(const char*, const char*, unsigned long);
extern unsigned long strlen(const char*);
extern char* strcpy(char*, const char*);
extern char* strncpy(char*, const char*, unsigned long);
extern char* strchr(const char*, int);
extern char* strrchr(const char*, int);
extern char* strcat(char*, const char*);
extern char* strncat(char*, const char*, unsigned long);
extern char* strstr(const char*, const char*);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int puts(const char*);
extern int putchar(int);
extern void* malloc(unsigned long);
extern void free(void*);
extern void* calloc(unsigned long, unsigned long);
extern void* realloc(void*, unsigned long);
extern int abs(int);
extern long labs(long);
extern int atoi(const char*);
extern long atol(const char*);
extern double atof(const char*);
extern double sqrt(double);
extern double fabs(double);
extern double pow(double, double);
extern double ceil(double);
extern double floor(double);
extern void exit(int);
extern void abort(void);
extern int rand(void);
extern void srand(unsigned int);

extern void abort (void);
void  vec_assert_fail (void)
{
    abort ();
}
struct ggc_root_tab {
    void *base;
};
typedef struct deferred_access_check {} VEC_deferred_access_check_gc;
typedef struct deferred_access {
    VEC_deferred_access_check_gc* deferred_access_checks;
    int deferring_access_checks_kind;
} deferred_access;
typedef struct VEC_deferred_access_base {
    unsigned num;
    deferred_access vec[1];
} VEC_deferred_access_base;
static  deferred_access * VEC_deferred_access_base_last(VEC_deferred_access_base *vec_)
{
    (void)((vec_ && vec_->num) ? 0 : (vec_assert_fail (), 0));
    return &vec_->vec[vec_->num - 1];
}
static  void VEC_deferred_access_base_pop(VEC_deferred_access_base *vec_)
{
    (void)((vec_->num) ? 0 : (vec_assert_fail (), 0));
    --vec_->num;
}
void  perform_access_checks (VEC_deferred_access_check_gc* p)
{
    abort ();
}
typedef struct VEC_deferred_access_gc {
    VEC_deferred_access_base base;
} VEC_deferred_access_gc;
static VEC_deferred_access_gc *deferred_access_stack;
static unsigned deferred_access_no_check;
const struct ggc_root_tab gt_pch_rs_gt_cp_semantics_h[] = {
    {
 &deferred_access_no_check
    }
};
void  pop_to_parent_deferring_access_checks (void)
{
    if (deferred_access_no_check)
 deferred_access_no_check--;
    else
    {
        VEC_deferred_access_check_gc *checks;
        deferred_access *ptr;
 checks = (VEC_deferred_access_base_last(deferred_access_stack ? &deferred_access_stack->base : 0))->deferred_access_checks;
        VEC_deferred_access_base_pop(deferred_access_stack ? &deferred_access_stack->base : 0);
        ptr = VEC_deferred_access_base_last(deferred_access_stack ? &deferred_access_stack->base : 0);
        if (ptr->deferring_access_checks_kind == 0)
     perform_access_checks (checks);
    }
}
int main()
{
    deferred_access_stack = malloc (sizeof(VEC_deferred_access_gc) + sizeof(deferred_access) * 8);
    deferred_access_stack->base.num = 2;
    deferred_access_stack->base.vec[0].deferring_access_checks_kind = 1;
    pop_to_parent_deferring_access_checks ();
    return 0;
}

