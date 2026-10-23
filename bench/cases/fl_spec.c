/* Fakelua specialized-table get/set: STRINGID switch on interned keys,
 * fallback size_+memcmp on VarString-like buffers (c_gen SpecGet/Set). */
package main;
import runtime;

enum {
    VAR_NIL = 0,
    VAR_INT = 2,
    VAR_STRING = 4,
    VAR_STRINGID = 5
};

typedef struct VarString {
    int size_;
    unsigned hash_;
    char data_[16];
} VarString;

typedef struct CVar {
    int type_;
    int flag_;
    union {
        long long i;
        VarString *s;
    } data_;
} CVar;

enum {
    SID_X = 1,
    SID_Y = 2,
    SID_Z = 3,
    SID_W = 4,
    SID_NAME = 5,
    SID_LEN = 6,
    SID_FLAG = 7,
    SID_NEXT = 8
};

typedef struct Spec {
    CVar x, y, z, w, name, len, flag, next;
} Spec;

enum { NKEYS = 8 };
enum { N = 3500000 };

static Spec spec;
static VarString str_pool[NKEYS];
static const char *lit[NKEYS] = { "x", "y", "z", "w", "name", "len", "flag", "next" };
static const int sids[NKEYS] = {
    SID_X, SID_Y, SID_Z, SID_W, SID_NAME, SID_LEN, SID_FLAG, SID_NEXT
};

static unsigned shash(const char *s, int n) {
    unsigned h = 2166136261u;
    int i;
    for (i = 0; i < n; i++) {
        h ^= (unsigned char)s[i];
        h *= 16777619u;
    }
    return h;
}

static int scmp_n(const char *a, const char *b, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i])
            return (unsigned char)a[i] - (unsigned char)b[i];
    }
    return 0;
}

static CVar spec_get(Spec *s, CVar k, int *finish) {
    CVar nilv;
    nilv.type_ = VAR_NIL;
    nilv.flag_ = 0;
    nilv.data_.i = 0;
    *finish = 0;
    if (k.type_ == VAR_STRINGID) {
        switch (k.data_.i) {
        case SID_X: *finish = 1; return s->x;
        case SID_Y: *finish = 1; return s->y;
        case SID_Z: *finish = 1; return s->z;
        case SID_W: *finish = 1; return s->w;
        case SID_NAME: *finish = 1; return s->name;
        case SID_LEN: *finish = 1; return s->len;
        case SID_FLAG: *finish = 1; return s->flag;
        case SID_NEXT: *finish = 1; return s->next;
        default: break;
        }
    } else if (k.type_ == VAR_STRING) {
        VarString *vs = k.data_.s;
        if (vs->size_ == 1 && scmp_n(vs->data_, "x", 1) == 0) { *finish = 1; return s->x; }
        if (vs->size_ == 1 && scmp_n(vs->data_, "y", 1) == 0) { *finish = 1; return s->y; }
        if (vs->size_ == 1 && scmp_n(vs->data_, "z", 1) == 0) { *finish = 1; return s->z; }
        if (vs->size_ == 1 && scmp_n(vs->data_, "w", 1) == 0) { *finish = 1; return s->w; }
        if (vs->size_ == 4 && scmp_n(vs->data_, "name", 4) == 0) { *finish = 1; return s->name; }
        if (vs->size_ == 3 && scmp_n(vs->data_, "len", 3) == 0) { *finish = 1; return s->len; }
        if (vs->size_ == 4 && scmp_n(vs->data_, "flag", 4) == 0) { *finish = 1; return s->flag; }
        if (vs->size_ == 4 && scmp_n(vs->data_, "next", 4) == 0) { *finish = 1; return s->next; }
    }
    return nilv;
}

static void spec_set(Spec *s, CVar k, CVar v, int *finish) {
    *finish = 0;
    if (k.type_ == VAR_STRINGID) {
        switch (k.data_.i) {
        case SID_X: s->x = v; *finish = 1; return;
        case SID_Y: s->y = v; *finish = 1; return;
        case SID_Z: s->z = v; *finish = 1; return;
        case SID_W: s->w = v; *finish = 1; return;
        case SID_NAME: s->name = v; *finish = 1; return;
        case SID_LEN: s->len = v; *finish = 1; return;
        case SID_FLAG: s->flag = v; *finish = 1; return;
        case SID_NEXT: s->next = v; *finish = 1; return;
        default: break;
        }
    } else if (k.type_ == VAR_STRING) {
        VarString *vs = k.data_.s;
        if (vs->size_ == 1 && scmp_n(vs->data_, "x", 1) == 0) { s->x = v; *finish = 1; return; }
        if (vs->size_ == 1 && scmp_n(vs->data_, "y", 1) == 0) { s->y = v; *finish = 1; return; }
        if (vs->size_ == 1 && scmp_n(vs->data_, "z", 1) == 0) { s->z = v; *finish = 1; return; }
        if (vs->size_ == 1 && scmp_n(vs->data_, "w", 1) == 0) { s->w = v; *finish = 1; return; }
        if (vs->size_ == 4 && scmp_n(vs->data_, "name", 4) == 0) { s->name = v; *finish = 1; return; }
        if (vs->size_ == 3 && scmp_n(vs->data_, "len", 3) == 0) { s->len = v; *finish = 1; return; }
        if (vs->size_ == 4 && scmp_n(vs->data_, "flag", 4) == 0) { s->flag = v; *finish = 1; return; }
        if (vs->size_ == 4 && scmp_n(vs->data_, "next", 4) == 0) { s->next = v; *finish = 1; return; }
    }
}

int main(void) {
    int i;
    unsigned acc;
    CVar key, val, got;
    int finish;

    for (i = 0; i < NKEYS; i++) {
        int n = 0;
        while (lit[i][n] != 0)
            n++;
        str_pool[i].size_ = n;
        str_pool[i].hash_ = shash(lit[i], n);
        {
            int k;
            for (k = 0; k < n; k++)
                str_pool[i].data_[k] = lit[i][k];
            str_pool[i].data_[n] = 0;
        }
    }
    {
        CVar z;
        z.type_ = VAR_INT;
        z.flag_ = 0;
        z.data_.i = 0;
        spec.x = z; spec.y = z; spec.z = z; spec.w = z;
        spec.name = z; spec.len = z; spec.flag = z; spec.next = z;
    }

    acc = 0;
    for (i = 0; i < N; i++) {
        int ix = i & (NKEYS - 1);
        val.type_ = VAR_INT;
        val.flag_ = 0;
        val.data_.i = (long long)(i * 31 + 7);
        if ((i & 3) != 0) {
            key.type_ = VAR_STRINGID;
            key.flag_ = 0;
            key.data_.i = (long long)sids[ix];
        } else {
            key.type_ = VAR_STRING;
            key.flag_ = 0;
            key.data_.s = &str_pool[ix];
        }
        if ((i & 1) == 0)
            spec_set(&spec, key, val, &finish);
        got = spec_get(&spec, key, &finish);
        if (finish && got.type_ == VAR_INT)
            acc += (unsigned)got.data_.i;
        else
            acc += str_pool[ix].hash_;
    }
    runtime.printf("fl_spec %u\n", acc);
    return 0;
}
