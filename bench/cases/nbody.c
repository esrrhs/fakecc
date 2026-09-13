/* Tiny n-body with Newton's sqrt. Double arithmetic, no libm. */
package main;
import runtime;

enum { BODIES = 5 };
enum { STEPS = 120000 };

static double dsqrt(double x) {
    double r;
    int i;
    if (x <= 0.0)
        return 0.0;
    r = x;
    for (i = 0; i < 20; i++)
        r = 0.5 * (r + x / r);
    return r;
}

int main(void) {
    double x[BODIES], y[BODIES], z[BODIES];
    double vx[BODIES], vy[BODIES], vz[BODIES];
    double m[BODIES];
    double dt, energy;
    int i, j, s;
    long long scaled;

    dt = 0.01;
    m[0] = 1.0;   x[0] = 0.0;  y[0] = 0.0;  z[0] = 0.0;
    m[1] = 0.1;   x[1] = 1.0;  y[1] = 0.0;  z[1] = 0.0;
    m[2] = 0.1;   x[2] = 0.0;  y[2] = 1.2;  z[2] = 0.0;
    m[3] = 0.05;  x[3] = -0.8; y[3] = 0.4;  z[3] = 0.3;
    m[4] = 0.05;  x[4] = 0.3;  y[4] = -0.9; z[4] = -0.2;

    vx[0] = 0.0;  vy[0] = 0.0;  vz[0] = 0.0;
    vx[1] = 0.0;  vy[1] = 0.8;  vz[1] = 0.1;
    vx[2] = -0.7; vy[2] = 0.0;  vz[2] = 0.2;
    vx[3] = 0.2;  vy[3] = -0.5; vz[3] = 0.4;
    vx[4] = 0.4;  vy[4] = 0.3;  vz[4] = -0.3;

    for (s = 0; s < STEPS; s++) {
        for (i = 0; i < BODIES; i++) {
            for (j = i + 1; j < BODIES; j++) {
                double dx = x[i] - x[j];
                double dy = y[i] - y[j];
                double dz = z[i] - z[j];
                double r2 = dx * dx + dy * dy + dz * dz + 1.0e-9;
                double r = dsqrt(r2);
                double mag = dt / (r2 * r);
                vx[i] -= dx * m[j] * mag;
                vy[i] -= dy * m[j] * mag;
                vz[i] -= dz * m[j] * mag;
                vx[j] += dx * m[i] * mag;
                vy[j] += dy * m[i] * mag;
                vz[j] += dz * m[i] * mag;
            }
        }
        for (i = 0; i < BODIES; i++) {
            x[i] += dt * vx[i];
            y[i] += dt * vy[i];
            z[i] += dt * vz[i];
        }
    }

    energy = 0.0;
    for (i = 0; i < BODIES; i++)
        energy += 0.5 * m[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
    scaled = (long long)(energy * 1000000.0);
    if (scaled < 0)
        scaled = -scaled;
    runtime.printf("nbody %lld\n", scaled);
    return 0;
}
