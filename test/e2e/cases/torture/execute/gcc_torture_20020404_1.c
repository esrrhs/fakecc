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
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
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


typedef long long bfd_signed_vma;
typedef bfd_signed_vma file_ptr;
typedef int boolean;
typedef unsigned long long bfd_size_type;
typedef unsigned int flagword;
typedef unsigned long long CORE_ADDR;
typedef unsigned long long bfd_vma;
struct bfd_struct { int x; };
struct asection_struct {
    unsigned int user_set_vma : 1;
    bfd_vma vma;
    bfd_vma lma;
    unsigned int alignment_power;
    unsigned int entsize;
};
typedef struct bfd_struct bfd;
typedef struct asection_struct asection;
static bfd foo_bfd;
static asection foo_section;
static bfd *bfd_openw_with_cleanup(char *filename, const char *target, char *mode) {
    foo_bfd.x = 0;
    return &foo_bfd;
}
static asection *bfd_make_section_anyway(bfd *abfd, const char *name) {
    foo_section.user_set_vma = 0;
    foo_section.vma = 0;
    foo_section.lma = 0;
    foo_section.alignment_power = 0;
    return &foo_section;
}
static boolean bfd_set_section_size(bfd *abfd, asection *sec, bfd_size_type val) { return 1; }
static boolean bfd_set_section_flags(bfd *abfd, asection *sec, flagword flags) { return 1; }
static boolean bfd_set_section_contents(bfd *abfd, asection *section, void *data, file_ptr offset, bfd_size_type count) {
    if (count != (bfd_size_type)0x1eadbeef) abort();
    return 1;
}
static void dump_bfd_file(char *filename, char *mode, char *target, CORE_ADDR vaddr, char *buf, int len) {
    bfd *obfd;
    asection *osection;
    obfd = bfd_openw_with_cleanup(filename, target, mode);
    osection = bfd_make_section_anyway(obfd, ".newsec");
    bfd_set_section_size(obfd, osection, len);
    osection->vma = osection->lma = vaddr;
    osection->user_set_vma = 1;
    osection->alignment_power = 0;
    bfd_set_section_flags(obfd, osection, 0x203);
    osection->entsize = 0;
    bfd_set_section_contents(obfd, osection, buf, 0, len);
}
static char hello[] = "hello";
int main(void) {
    dump_bfd_file(0, 0, 0, (CORE_ADDR)0xdeadbeef, hello, (int)0x1eadbeef);
    return 0;
}
