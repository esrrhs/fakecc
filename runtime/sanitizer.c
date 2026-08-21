package runtime;

static int g_asan_inited;

static void asan_write_str(int fd, const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    __syscall(1, fd, (long)s, (long)len, 0, 0, 0);
}

static void asan_write_hex(int fd, unsigned long val) {
    char buf[20];
    buf[0] = '0';
    buf[1] = 'x';
    if (val == 0) {
        buf[2] = '0';
        buf[3] = '\0';
        asan_write_str(fd, buf);
        return;
    }
    const char *hex = "0123456789abcdef";
    int idx = 18;
    buf[19] = '\0';
    while (val > 0 && idx >= 2) {
        buf[idx] = hex[val & 0xf];
        idx = idx - 1;
        val = val >> 4;
    }
    asan_write_str(fd, &buf[idx + 1]);
}

static void asan_write_dec(int fd, unsigned long val) {
    char buf[24];
    if (val == 0) {
        asan_write_str(fd, "0");
        return;
    }
    int idx = 22;
    buf[23] = '\0';
    while (val > 0 && idx >= 0) {
        buf[idx] = (char)('0' + (val % 10));
        idx = idx - 1;
        val = val / 10;
    }
    asan_write_str(fd, &buf[idx + 1]);
}

void __asan_init(void) {
    if (g_asan_inited) return;
    g_asan_inited = 1;
    /* Map 16TB shadow memory at 0x100000000000 with MAP_NORESERVE | MAP_FIXED (0x4032) */
    long p = __syscall(9, 0x100000000000L, 0x100000000000L, 3, 0x4032, -1, 0);
    if (p < 0) {
        p = __syscall(9, 0x100000000000L, 0x10000000000L, 3, 0x4032, -1, 0);
    }
}

void __asan_poison_memory_region(void *addr, size_t size) {
    if (!g_asan_inited) __asan_init();
    if (size == 0 || addr == 0) return;
    unsigned long u = (unsigned long)addr;
    unsigned long shadow_start = (u >> 3) + 0x100000000000UL;
    unsigned long shadow_end = ((u + size + 7) >> 3) + 0x100000000000UL;
    char *s = (char *)shadow_start;
    char *e = (char *)shadow_end;
    while (s < e) {
        *s = (char)0xff;
        s = s + 1;
    }
}

void __asan_unpoison_memory_region(void *addr, size_t size) {
    if (!g_asan_inited) __asan_init();
    if (size == 0 || addr == 0) return;
    unsigned long u = (unsigned long)addr;
    unsigned long shadow_start = (u >> 3) + 0x100000000000UL;
    unsigned long shadow_end = ((u + size) >> 3) + 0x100000000000UL;
    char *s = (char *)shadow_start;
    char *e = (char *)shadow_end;
    while (s < e) {
        *s = 0;
        s = s + 1;
    }
    size_t rem = (u + size) & 7;
    if (rem > 0) {
        char *last = (char *)shadow_end;
        *last = (char)rem;
    }
}

void __asan_report_error(void *addr, size_t size, int is_write) {
    asan_write_str(2, "=================================================================\n");
    asan_write_str(2, "==FAKECC-ASAN==ERROR: AddressSanitizer: out-of-bounds access on address ");
    asan_write_hex(2, (unsigned long)addr);
    asan_write_str(2, "\n");
    if (is_write) {
        asan_write_str(2, "WRITE of size ");
    } else {
        asan_write_str(2, "READ of size ");
    }
    asan_write_dec(2, size);
    asan_write_str(2, " at address ");
    asan_write_hex(2, (unsigned long)addr);
    asan_write_str(2, "\n");
    asan_write_str(2, "=================================================================\n");
    __syscall(231, 1, 0, 0, 0, 0, 0);
}

static void asan_check_range(void *addr, size_t size, int is_write) {
    if (!g_asan_inited) __asan_init();
    unsigned long u = (unsigned long)addr;
    if (u < 0x400000UL) {
        __asan_report_error(addr, size, is_write);
        return;
    }
    if (u >= 0x100000000000UL && u < 0x200000000000UL) return;

    unsigned long shadow_start = (u >> 3) + 0x100000000000UL;
    unsigned long shadow_end = ((u + size - 1) >> 3) + 0x100000000000UL;
    char *s = (char *)shadow_start;
    char *e = (char *)shadow_end;
    while (s <= e) {
        char val = *s;
        if (val != 0) {
            unsigned long chunk_addr = ((unsigned long)(s - (char *)0x100000000000UL)) << 3;
            if (val < 0 || ((u + size > chunk_addr + (unsigned long)val) && (u < chunk_addr + 8))) {
                __asan_report_error(addr, size, is_write);
                return;
            }
        }
        s = s + 1;
    }
}

void __asan_load1(void *addr) { asan_check_range(addr, 1, 0); }
void __asan_load2(void *addr) { asan_check_range(addr, 2, 0); }
void __asan_load4(void *addr) { asan_check_range(addr, 4, 0); }
void __asan_load8(void *addr) { asan_check_range(addr, 8, 0); }
void __asan_loadN(void *addr, size_t size) { asan_check_range(addr, size, 0); }

void __asan_store1(void *addr) { asan_check_range(addr, 1, 1); }
void __asan_store2(void *addr) { asan_check_range(addr, 2, 1); }
void __asan_store4(void *addr) { asan_check_range(addr, 4, 1); }
void __asan_store8(void *addr) { asan_check_range(addr, 8, 1); }
void __asan_storeN(void *addr, size_t size) { asan_check_range(addr, size, 1); }
