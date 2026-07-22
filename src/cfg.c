#include "fakecc/cfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Small generic helpers                                               */
/* ------------------------------------------------------------------ */

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    return p;
}

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    return q;
}

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

static void cfg_init(CFG *g) {
    g->blocks = NULL;
    g->num = 0;
    g->entry = 0;
}

static CFGBlock *cfg_new_block(CFG *g, size_t start) {
    g->blocks = xrealloc(g->blocks, (g->num + 1) * sizeof(CFGBlock));
    CFGBlock *b = &g->blocks[g->num];
    b->id = (int)g->num;
    b->label = -1;
    b->start = start;
    b->end = start;
    b->preds = NULL;
    b->num_preds = 0;
    b->succs = NULL;
    b->num_succs = 0;
    g->num++;
    return b;
}

static void block_add_pred(CFGBlock *b, int pred) {
    for (size_t i = 0; i < b->num_preds; i++)
        if (b->preds[i] == pred) return;
    b->preds = xrealloc(b->preds, (b->num_preds + 1) * sizeof(int));
    b->preds[b->num_preds++] = pred;
}

static void block_add_succ(CFGBlock *b, int succ) {
    for (size_t i = 0; i < b->num_succs; i++)
        if (b->succs[i] == succ) return;
    b->succs = xrealloc(b->succs, (b->num_succs + 1) * sizeof(int));
    b->succs[b->num_succs++] = succ;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void cfg_link(CFG *g, int from, int to) {
    block_add_succ(&g->blocks[from], to);
    block_add_pred(&g->blocks[to], from);
}

void cfg_free(CFG *g) {
    for (size_t i = 0; i < g->num; i++) {
        free(g->blocks[i].preds);
        free(g->blocks[i].succs);
    }
    free(g->blocks);
    g->blocks = NULL;
    g->num = 0;
}

int cfg_find_label(const CFG *g, int label) {
    for (size_t i = 0; i < g->num; i++)
        if (g->blocks[i].label == label)
            return (int)i;
    return -1;
}

void cfg_build(CFG *g, const IRInstArray *insts) {
    cfg_init(g);
    if (insts->len == 0) {
        cfg_new_block(g, 0);
        return;
    }

    /* Pass 1: identify leaders and create blocks. */
    CFGBlock *cur = cfg_new_block(g, 0);
    for (size_t i = 0; i < insts->len; i++) {
        IRInst *inst = &insts->data[i];
        if (inst->op == IR_LABEL) {
            /* A label is a leader: close the previous block (if non-empty)
             * and start a new one. */
            if (cur->end > cur->start) {
                cur = cfg_new_block(g, i);
            }
            cur->label = inst->imm;
            cur->end = i + 1;
            continue;
        }
        cur->end = i + 1;
        if (inst->op == IR_BR || inst->op == IR_CBR || inst->op == IR_RETURN) {
            /* Terminator ends the block */
            if (i + 1 < insts->len) {
                cur = cfg_new_block(g, i + 1);
            }
        }
    }

    /* Pass 2: link successors. Fall-through applies when a block does not
     * end in an unconditional terminator. */
    for (size_t bi = 0; bi < g->num; bi++) {
        CFGBlock *b = &g->blocks[bi];
        if (b->start >= b->end) {
            /* Empty block: falls through to next */
            if (bi + 1 < g->num)
                cfg_link(g, (int)bi, (int)bi + 1);
            continue;
        }
        IRInst *last = &insts->data[b->end - 1];
        switch (last->op) {
        case IR_BR: {
            int t = cfg_find_label(g, last->imm);
            if (t >= 0) cfg_link(g, (int)bi, t);
            break;
        }
        case IR_CBR: {
            int t = cfg_find_label(g, last->imm);
            int f = cfg_find_label(g, last->b);
            if (t >= 0) cfg_link(g, (int)bi, t);
            if (f >= 0) cfg_link(g, (int)bi, f);
            break;
        }
        case IR_RETURN:
            /* No successor */
            break;
        default:
            /* Fall through to next block */
            if (bi + 1 < g->num)
                cfg_link(g, (int)bi, (int)bi + 1);
            break;
        }
    }
}

int *cfg_rpo(const CFG *g) {
    size_t n = g->num;
    if (n == 0) return NULL;

    char *visited = calloc(n, 1);
    if (!visited) return NULL;

    int *postorder = xmalloc(n * sizeof(int));
    size_t po_len = 0;

    /* Iterative DFS stacks: one for block ids, one for next-succ indices */
    int *stack_blocks = xmalloc(n * sizeof(int));
    size_t *stack_next = xmalloc(n * sizeof(size_t));
    size_t stack_len = 0;

    /* Push entry block */
    stack_blocks[0] = g->entry;
    stack_next[0] = 0;
    stack_len = 1;
    visited[g->entry] = 1;

    while (stack_len > 0) {
        int b = stack_blocks[stack_len - 1];
        size_t ci = stack_next[stack_len - 1];
        CFGBlock *blk = &g->blocks[b];

        /* Find next unvisited successor */
        int found = 0;
        while (ci < blk->num_succs) {
            int s = blk->succs[ci];
            if (!visited[s]) {
                visited[s] = 1;
                stack_next[stack_len - 1] = ci + 1;
                stack_blocks[stack_len] = s;
                stack_next[stack_len] = 0;
                stack_len++;
                found = 1;
                break;
            }
            ci++;
        }
        if (!found) {
            /* All successors processed — add to postorder */
            postorder[po_len++] = b;
            stack_len--;
        }
    }

    free(visited);
    free(stack_blocks);
    free(stack_next);

    /* Build RPO: rpo[block_id] = position in reverse-postorder traversal.
     * Entry block (last in postorder) gets RPO 0 — required by domtree
     * intersect (idom must have a smaller RPO). */
    int *rpo = xmalloc(n * sizeof(int));
    for (size_t i = 0; i < n; i++)
        rpo[postorder[n - 1 - i]] = (int)i;
    free(postorder);

    return rpo;
}
