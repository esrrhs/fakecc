#ifndef FAKECC_DFP_H
#define FAKECC_DFP_H

/* Compile-time IEEE 754 BID decimal floating-point (host, not runtime). */

int dfp_from_str(const char *text, int width,
                 unsigned long long *lo, unsigned long long *hi);
void dfp_neg_bits(int width, unsigned long long *lo, unsigned long long *hi);
int dfp_binop_bits(int op, int width,
                   unsigned long long alo, unsigned long long ahi,
                   unsigned long long blo, unsigned long long bhi,
                   unsigned long long *lo, unsigned long long *hi);
int dfp_cmp_bits(int width,
                 unsigned long long alo, unsigned long long ahi,
                 unsigned long long blo, unsigned long long bhi);
int dfp_from_int(unsigned long long val, unsigned long long val_hi,
                 int is_unsigned, int width,
                 unsigned long long *lo, unsigned long long *hi);
int dfp_convert_width(int src_w, unsigned long long slo, unsigned long long shi,
                      int dst_w, unsigned long long *dlo, unsigned long long *dhi);

/* op: 0=add 1=sub 2=mul 3=div */
enum { DFP_ADD = 0, DFP_SUB = 1, DFP_MUL = 2, DFP_DIV = 3 };

#endif
