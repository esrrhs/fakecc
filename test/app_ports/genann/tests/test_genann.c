// expect: 0
package main;

import genann;
import runtime;

extern double exp(double x);
extern double tanh(double x);
extern double sin(double x);
extern double fabs(double x);
extern int rand(void);
extern void srand(unsigned int seed);

/* Minimal test helpers (exit-code only; mirrors genann's minctest.h). */
static int lfails = 0;
static void lequal(long long a, long long b) {
    if (a != b) {
        ++lfails;
        runtime.printf("assertion failed: %lld != %lld\n", a, b);
    }
}
static void lfequal(double a, double b) {
    if (fabs(a - b) > 0.001) {
        ++lfails;
        runtime.printf("assertion failed: %f != %f\n", a, b);
    }
}

int main(void) {
    srand(100); // Repeatable seed

    // Test 1: Basic linear neuron (similar to genann's basic() test)
    {
        genann.genann *ann = genann.genann_init(1, 0, 0, 1);
        if (!ann) {
            runtime.printf("FAIL: basic: failed to init ann\n");
            return 1;
        }
        lequal(ann->total_weights, 2);
        double a;
        a = 0;
        ann->weight[0] = 0;
        ann->weight[1] = 0;
        double output = *genann.genann_run(ann, &a);
        if (output < 0.49 || output > 0.51) {
            runtime.printf("FAIL: basic: input 0.0, expected ~0.5, got %f\n", output);
            genann.genann_free(ann);
            return 1;
        }
        a = 1;
        output = *genann.genann_run(ann, &a);
        if (output < 0.49 || output > 0.51) {
            runtime.printf("FAIL: basic: input 1.0, expected ~0.5, got %f\n", output);
            genann.genann_free(ann);
            return 1;
        }
        a = 11;
        output = *genann.genann_run(ann, &a);
        if (output < 0.49 || output > 0.51) {
            runtime.printf("FAIL: basic: input 11.0, expected ~0.5, got %f\n", output);
            genann.genann_free(ann);
            return 1;
        }
        a = 1;
        ann->weight[0] = 1;
        ann->weight[1] = 1;
        output = *genann.genann_run(ann, &a);
        if (output < 0.49 || output > 0.51) {
            runtime.printf("FAIL: basic: input 1.0 with weights [1,1], expected ~0.5, got %f\n", output);
            genann.genann_free(ann);
            return 1;
        }
        a = 10;
        ann->weight[0] = 1;
        ann->weight[1] = 1;
        output = *genann.genann_run(ann, &a);
        if (output < 0.99 || output > 1.01) {
            runtime.printf("FAIL: basic: input 10.0 with weights [1,1], expected ~1.0, got %f\n", output);
            genann.genann_free(ann);
            return 1;
        }
        a = -10;
        ann->weight[0] = 1;
        ann->weight[1] = 1;
        output = *genann.genann_run(ann, &a);
        if (output < -0.01 || output > 0.01) {
            runtime.printf("FAIL: basic: input -10.0 with weights [1,1], expected ~0.0, got %f\n", output);
            genann.genann_free(ann);
            return 1;
        }
        genann.genann_free(ann);
    }

    // Test 2: XOR network (similar to genann's xor() test)
    {
        genann.genann *ann = genann.genann_init(2, 1, 2, 1);
        if (!ann) {
            runtime.printf("FAIL: xor: failed to init ann\n");
            return 1;
        }
        lequal(ann->total_weights, 9);
        // First hidden neuron.
        ann->weight[0] = 0.5;   // bias
        ann->weight[1] = 1.0;   // weight from input 0
        ann->weight[2] = 1.0;   // weight from input 1
        // Second hidden neuron.
        ann->weight[3] = 1.0;   // bias
        ann->weight[4] = 1.0;   // weight from input 0
        ann->weight[5] = 1.0;   // weight from input 1
        // Output neuron.
        ann->weight[6] = 0.5;   // bias
        ann->weight[7] = 1.0;   // weight from hidden 0
        ann->weight[8] = -1.0;  // weight from hidden 1

        ann->activation_hidden = genann.genann_act_threshold;
        ann->activation_output = genann.genann_act_threshold;

        double input[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
        double expected[4] = {0, 1, 1, 0};

        int i;
        for (i = 0; i < 4; i++) {
            double out = *genann.genann_run(ann, input[i]);
            if (out < 0.0 || out > 1.0) {
                runtime.printf("FAIL: xor: output out of range for input [%d,%d], got %f\n",
                               (int)input[i][0], (int)input[i][1], out);
                genann.genann_free(ann);
                return 1;
            }
            int expected_val = (int)expected[i];
            int got = (out > 0.5) ? 1 : 0;
            if (got != expected_val) {
                runtime.printf("FAIL: xor: expected %d for input [%d,%d], got %d (out=%f)\n",
                               expected_val, (int)input[i][0], (int)input[i][1], got, out);
                genann.genann_free(ann);
                return 1;
            }
        }
        genann.genann_free(ann);
    }

    // Test 3: Backprop (similar to genann's backprop() test)
    {
        genann.genann *ann = genann.genann_init(1, 0, 0, 1);
        if (!ann) {
            runtime.printf("FAIL: backprop: failed to init ann\n");
            return 1;
        }
        double input = 0.5;
        double target = 1.0;
        double first_try = *genann.genann_run(ann, &input);
        genann.genann_train(ann, &input, &target, 0.5);
        double second_try = *genann.genann_run(ann, &input);
        if (!(fabs(first_try - target) > fabs(second_try - target))) {
            runtime.printf("FAIL: backprop: expected improvement after training, first=%.6f second=%.6f target=%.6f\n",
                           first_try, second_try, target);
            genann.genann_free(ann);
            return 1;
        }
        genann.genann_free(ann);
    }

    // Test 4: Train AND
    {
        double input[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
        double target[4] = {0, 0, 0, 1};
        genann.genann *ann = genann.genann_init(2, 0, 0, 1);
        if (!ann) {
            runtime.printf("FAIL: train_and: failed to init ann\n");
            return 1;
        }
        int i, j;
        for (i = 0; i < 50; ++i)
            for (j = 0; j < 4; ++j)
                genann.genann_train(ann, input[j], target + j, 0.8);
        ann->activation_output = genann.genann_act_threshold;
        for (i = 0; i < 4; i++) {
            double out = *genann.genann_run(ann, input[i]);
            int expected = (int)target[i];
            int got = (out > 0.5) ? 1 : 0;
            if (got != expected) {
                runtime.printf("FAIL: train_and: expected %d for input [%d,%d], got %d (out=%f)\n",
                               expected, (int)input[i][0], (int)input[i][1], got, out);
                genann.genann_free(ann);
                return 1;
            }
        }
        genann.genann_free(ann);
    }

    // Test 5: Train OR
    {
        double input[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
        double target[4] = {0, 1, 1, 1};
        genann.genann *ann = genann.genann_init(2, 0, 0, 1);
        if (!ann) {
            runtime.printf("FAIL: train_or: failed to init ann\n");
            return 1;
        }
        genann.genann_randomize(ann);
        int i, j;
        for (i = 0; i < 50; ++i)
            for (j = 0; j < 4; ++j)
                genann.genann_train(ann, input[j], target + j, 0.8);
        ann->activation_output = genann.genann_act_threshold;
        for (i = 0; i < 4; i++) {
            double out = *genann.genann_run(ann, input[i]);
            int expected = (int)target[i];
            int got = (out > 0.5) ? 1 : 0;
            if (got != expected) {
                runtime.printf("FAIL: train_or: expected %d for input [%d,%d], got %d (out=%f)\n",
                               expected, (int)input[i][0], (int)input[i][1], got, out);
                genann.genann_free(ann);
                return 1;
            }
        }
        genann.genann_free(ann);
    }

    // Test 6: Train XOR
    {
        double input[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
        double target[4] = {0, 1, 1, 0};
        genann.genann *ann = genann.genann_init(2, 1, 2, 1);
        if (!ann) {
            runtime.printf("FAIL: train_xor: failed to init ann\n");
            return 1;
        }
        int i, j;
        for (i = 0; i < 500; ++i)
            for (j = 0; j < 4; ++j)
                genann.genann_train(ann, input[j], target + j, 3);
        ann->activation_output = genann.genann_act_threshold;
        for (i = 0; i < 4; i++) {
            double out = *genann.genann_run(ann, input[i]);
            int expected = (int)target[i];
            int got = (out > 0.5) ? 1 : 0;
            if (got != expected) {
                runtime.printf("FAIL: train_xor: expected %d for input [%d,%d], got %d (out=%f)\n",
                               expected, (int)input[i][0], (int)input[i][1], got, out);
                genann.genann_free(ann);
                return 1;
            }
        }
        genann.genann_free(ann);
    }

    // Test 7: Copy
    {
        genann.genann *first = genann.genann_init(10, 1, 8, 5);
        if (!first) {
            runtime.printf("FAIL: copy: failed to init first ann\n");
            return 1;
        }
        genann.genann *second = genann.genann_copy(first);
        if (!second) {
            runtime.printf("FAIL: copy: failed to copy ann\n");
            genann.genann_free(first);
            return 1;
        }
        if (first->inputs != second->inputs ||
            first->hidden_layers != second->hidden_layers ||
            first->hidden != second->hidden ||
            first->outputs != second->outputs ||
            first->total_weights != second->total_weights) {
            runtime.printf("FAIL: copy: structural mismatch after copy\n");
            genann.genann_free(first);
            genann.genann_free(second);
            return 1;
        }
        int i;
        for (i = 0; i < first->total_weights; ++i) {
            if (fabs(first->weight[i] - second->weight[i]) > 0.000001) {
                runtime.printf("FAIL: copy: weight mismatch at index %d\n", i);
                genann.genann_free(first);
                genann.genann_free(second);
                return 1;
            }
        }
        genann.genann_free(first);
        genann.genann_free(second);
    }

    // Test 8: Sigmoid function
    {
        double i = -20;
        const double max = 20;
        const double d = 0.0001;
        while (i < max) {
            double val1 = genann.genann_act_sigmoid(0, i);
            double val2 = genann.genann_act_sigmoid_cached(0, i);
            if (fabs(val1 - val2) > 0.001) {
                runtime.printf("FAIL: sigmoid: mismatch at i=%f, cached=%f direct=%f\n", i, val2, val1);
                return 1;
            }
            i += d;
        }
    }

    // All tests passed
    return lfails != 0;
}