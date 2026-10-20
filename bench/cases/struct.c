/* Nested structs, pass by value, return by value. */
package main;
import runtime;

struct Vec { int x, y, z, w; };

struct Body {
    struct Vec p;
    struct Vec v;
    int mass;
};

enum { N = 6000 };
enum { STEPS = 600 };

static struct Body bodies[N];

static struct Vec addv(struct Vec a, struct Vec b) {
    struct Vec r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    r.w = a.w + b.w;
    return r;
}

int main(void) {
    int i, step;
    unsigned sum;

    for (i = 0; i < N; i++) {
        bodies[i].p.x = i * 3 - 100;
        bodies[i].p.y = i * 5 + 7;
        bodies[i].p.z = i & 31;
        bodies[i].p.w = 1;
        bodies[i].v.x = (i & 7) - 3;
        bodies[i].v.y = i & 3;
        bodies[i].v.z = 1;
        bodies[i].v.w = 0;
        bodies[i].mass = (i & 15) + 1;
    }
    for (step = 0; step < STEPS; step++) {
        for (i = 0; i < N; i++) {
            struct Vec nv = addv(bodies[i].p, bodies[i].v);
            bodies[i].p = nv;
            bodies[i].v.x += bodies[i].mass - 8;
        }
    }
    sum = 0;
    for (i = 0; i < N; i++)
        sum += (unsigned)bodies[i].p.x + (unsigned)bodies[i].p.y + (unsigned)bodies[i].p.z;
    runtime.printf("struct %u\n", sum);
    return 0;
}
