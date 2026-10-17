#ifndef FAKECC_SHA256_H
#define FAKECC_SHA256_H

#include <stddef.h>
#include <stdint.h>

/* SHA-256 (FIPS 180-4).  The arm64 Mach-O writer needs it to build the ad-hoc
 * code signature the kernel requires before it will run an image: the file is
 * hashed one page at a time into a CodeDirectory.  Nothing here reaches for
 * libc beyond memcpy, so the selfhost image can sign its own output instead of
 * shelling out to codesign(1). */

#define SHA256_DIGEST_LEN 32

typedef struct {
    uint32_t h[8];
    uint64_t bit_len;
    unsigned char block[64];
    size_t used;
} SHA256Ctx;

void sha256_init(SHA256Ctx *ctx);
void sha256_update(SHA256Ctx *ctx, const void *data, size_t len);
void sha256_final(SHA256Ctx *ctx, unsigned char out[SHA256_DIGEST_LEN]);

/* One-shot convenience wrapper, the form the signature builder wants. */
void sha256_buf(const void *data, size_t len,
                unsigned char out[SHA256_DIGEST_LEN]);

#endif /* FAKECC_SHA256_H */
