/* Fakelua VarTable quick path: fixed quick_data_[8] plus open-addressed
 * int-key nodes, array part for 1..arr_size_, seq-len cache touch. */
package main;
import runtime;

typedef struct CVar {
    int type_;
    int flag_;
    union {
        long long i;
        unsigned long long u;
    } data_;
} CVar;

typedef struct VarEntry {
    CVar key;
    CVar val;
    unsigned hash;
} VarEntry;

typedef struct TableNode {
    VarEntry entry;
    unsigned next;
    unsigned active_pos;
} TableNode;

enum { QUICK = 8 };
enum { BUCKETS = 256 };
enum { ARR = 128 };
enum { OPS = 2500000 };

static VarEntry quick_data_[QUICK];
static unsigned quick_count;
static TableNode nodes_[BUCKETS];
static CVar arr_[ARR];
static unsigned arr_size_;
static unsigned seq_len_valid_;
static long long seq_len_;

static unsigned hash_int(long long k) {
    return (unsigned)(k ^ (k >> 32));
}

static void table_clear(void) {
    int i;
    quick_count = 0;
    for (i = 0; i < QUICK; i++) {
        quick_data_[i].key.type_ = 0;
        quick_data_[i].val.type_ = 0;
    }
    for (i = 0; i < BUCKETS; i++) {
        nodes_[i].entry.key.type_ = 0;
        nodes_[i].entry.val.type_ = 0;
        nodes_[i].next = 0xFFFFFFFFu;
        nodes_[i].active_pos = 0;
    }
    for (i = 0; i < ARR; i++) {
        arr_[i].type_ = 0;
        arr_[i].data_.i = 0;
    }
    arr_size_ = ARR;
    seq_len_valid_ = 0;
    seq_len_ = 0;
}

static CVar get_int(long long k) {
    unsigned i;
    CVar nilv;
    nilv.type_ = 0;
    nilv.flag_ = 0;
    nilv.data_.i = 0;
    if ((unsigned long long)k - 1ull < (unsigned long long)arr_size_)
        return arr_[k - 1];
    if (quick_count > 0) {
        for (i = 0; i < quick_count; i++) {
            if (quick_data_[i].key.type_ == 2 && quick_data_[i].key.data_.i == k)
                return quick_data_[i].val;
        }
    }
    {
        unsigned h = hash_int(k);
        unsigned mask = BUCKETS - 1;
        TableNode *curr = &nodes_[h & mask];
        while (1) {
            if (curr->entry.key.type_ == 2 && curr->entry.key.data_.i == k)
                return curr->entry.val;
            if (curr->entry.key.type_ == 0)
                return nilv;
            if (curr->next == 0xFFFFFFFFu)
                return nilv;
            curr = &nodes_[curr->next];
        }
    }
}

static void set_int(long long k, long long v) {
    unsigned i;
    CVar kv, vv;
    kv.type_ = 2;
    kv.flag_ = 0;
    kv.data_.i = k;
    vv.type_ = 2;
    vv.flag_ = 0;
    vv.data_.i = v;
    if ((unsigned long long)k - 1ull < (unsigned long long)arr_size_) {
        arr_[k - 1] = vv;
        seq_len_valid_ = 0;
        return;
    }
    if (quick_count < QUICK) {
        for (i = 0; i < quick_count; i++) {
            if (quick_data_[i].key.type_ == 2 && quick_data_[i].key.data_.i == k) {
                quick_data_[i].val = vv;
                return;
            }
        }
        quick_data_[quick_count].key = kv;
        quick_data_[quick_count].val = vv;
        quick_data_[quick_count].hash = hash_int(k);
        quick_count++;
        return;
    }
    {
        unsigned h = hash_int(k);
        unsigned mask = BUCKETS - 1;
        unsigned idx = h & mask;
        TableNode *curr = &nodes_[idx];
        if (curr->entry.key.type_ == 0) {
            curr->entry.key = kv;
            curr->entry.val = vv;
            curr->entry.hash = h;
            curr->next = 0xFFFFFFFFu;
            return;
        }
        if (curr->entry.key.type_ == 2 && curr->entry.key.data_.i == k) {
            curr->entry.val = vv;
            return;
        }
        /* Linear probe into free slot and link. */
        for (i = 1; i < BUCKETS; i++) {
            unsigned ni = (idx + i) & mask;
            if (nodes_[ni].entry.key.type_ == 0) {
                nodes_[ni].entry.key = kv;
                nodes_[ni].entry.val = vv;
                nodes_[ni].entry.hash = h;
                nodes_[ni].next = 0xFFFFFFFFu;
                curr->next = ni;
                return;
            }
            if (nodes_[ni].entry.key.type_ == 2 && nodes_[ni].entry.key.data_.i == k) {
                nodes_[ni].entry.val = vv;
                return;
            }
        }
    }
}

static long long seq_len(void) {
    long long n;
    if (seq_len_valid_)
        return seq_len_;
    n = 0;
    while (n < (long long)arr_size_ && arr_[n].type_ != 0)
        n++;
    seq_len_ = n;
    seq_len_valid_ = 1;
    return n;
}

int main(void) {
    unsigned i;
    unsigned acc;
    CVar g;

    table_clear();
    acc = 0;
    for (i = 0; i < OPS; i++) {
        long long k = (long long)((i * 17 + 3) % 400);
        long long v = (long long)(i * 13 + 7);
        if ((i & 3u) != 3u)
            set_int(k + 1, v);
        g = get_int(((i * 9) % 400) + 1);
        if (g.type_ == 2)
            acc += (unsigned)g.data_.i;
        if ((i & 127u) == 0u)
            acc += (unsigned)seq_len();
    }
    runtime.printf("fl_table %u\n", acc);
    return 0;
}
