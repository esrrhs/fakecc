/*
 * GENANN - Minimal C Artificial Neural Network
 * FakeCC port
 *
 * Copyright (c) 2015-2018 Lewis Van Winkle
 * Adapted for FakeCC by [porter]
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

package genann;

/* Import FakeCC runtime for standard types and core functions */
import runtime;

/* Declare needed libc/libm functions that we'll link with -lc -lm */
extern double exp(double x);
extern double tanh(double x);
extern int rand(void);

/* Constants - FakeCC doesn't have the C preprocessor, so we use enum */
enum { LOOKUP_SIZE = 4096, GENANN_MAX_DIMENSION = 1 << 20, RAND_MAX = 2147483647, INT_MAX = 2147483647 };

/* Global variables for sigmoid lookup table */
static double interval;
static double lookup[LOOKUP_SIZE];

/* Forward declarations of activation functions */
static double genann_act_sigmoid(const struct genann *ann, double a);
static double genann_act_sigmoid_cached(const struct genann *ann, double a);
static double genann_act_threshold(const struct genann *ann, double a);
static double genann_act_linear(const struct genann *ann, double a);
static double genann_act_tanh(const struct genann *ann, double a);
static double genann_act_relu(const struct genann *ann, double a);

/* Activation function type */
typedef double (*genann_actfun)(const struct genann *ann, double a);

/* The artificial neural network structure */
struct genann {
    /* How many inputs, outputs, and hidden neurons. */
    int inputs, hidden_layers, hidden, outputs;

    /* Which activation function to use for hidden neurons. Default: gennann_act_sigmoid_cached*/
    genann_actfun activation_hidden;

    /* Which activation function to use for output. Default: gennann_act_sigmoid_cached*/
    genann_actfun activation_output;

    /* Total number of weights, and size of weights buffer. */
    int total_weights;

    /* Total number of neurons + inputs and size of output buffer. */
    int total_neurons;

    /* All weights (total_weights long). */
    double *weight;

    /* Stores input array and output of each neuron (total_neurons long). */
    double *output;

    /* Stores delta of each hidden and output neuron (total_neurons - inputs long). */
    double *delta;
};

/* Convenience typedef */
typedef struct genann genann;

/* Creates and returns a new ann. */
genann *genann_init(int inputs, int hidden_layers, int hidden, int outputs);

/* Creates ANN from file saved with genann_write. */
genann *genann_read(runtime.FILE *in);

/* Sets weights randomly. Called by init. */
void genann_randomize(genann *ann);

/* Returns a new copy of ann. */
genann *genann_copy(genann const *ann);

/* Frees the memory used by an ann. */
void genann_free(genann *ann);

/* Runs the feedforward algorithm to calculate the ann's output. */
double const *genann_run(genann const *ann, double const *inputs);

/* Does a single backprop update. */
void genann_train(genann const *ann, double const *inputs, double const *desired_outputs, double learning_rate);

/* Saves the ann. */
void genann_write(genann const *ann, runtime.FILE *out);

/* Helper: initialize the sigmoid lookup table. */
void genann_init_sigmoid_lookup(const genann *ann);

/* Sigmoid activation function. */
static double genann_act_sigmoid(const struct genann *ann, double a) {
    if (a < -45.0) return 0.0;
    if (a > 45.0) return 1.0;
    return 1.0 / (1.0 + exp(-a));
}

/* Initialize the lookup table for cached sigmoid. */
void genann_init_sigmoid_lookup(const genann *ann) {
    const double f = (15.0 - (-15.0)) / LOOKUP_SIZE;
    int i;

    /* interval = LOOKUP_SIZE / (sigmoid_dom_max - sigmoid_dom_min) */
    /* We hardcode the domain [-15, 15] */
    interval = LOOKUP_SIZE / 30.0;
    for (i = 0; i < LOOKUP_SIZE; ++i) {
        lookup[i] = genann_act_sigmoid(ann, -15.0 + f * i);
    }
}

/* Cached sigmoid activation function. */
static double genann_act_sigmoid_cached(const struct genann *ann, double a) {
    /* Hardcoded domain [-15, 15] */
    if (a < -15.0) return lookup[0];
    if (a >= 15.0) return lookup[LOOKUP_SIZE - 1];

    runtime.size_t j = (runtime.size_t)((a - (-15.0)) * interval + 0.5);

    /* Because floating point... */
    if (j >= LOOKUP_SIZE) return lookup[LOOKUP_SIZE - 1];

    return lookup[j];
}

/* Linear activation function. */
static double genann_act_linear(const struct genann *ann, double a) {
    return a;
}

/* Threshold activation function. */
static double genann_act_threshold(const struct genann *ann, double a) {
    return a > 0 ? 1.0 : 0.0;
}

/* Hyperbolic tangent activation function. */
static double genann_act_tanh(const struct genann *ann, double a) {
    return tanh(a);
}

/* Rectified linear unit activation function. */
static double genann_act_relu(const struct genann *ann, double a) {
    return a > 0 ? a : 0;
}

/* Derivative of an activation function, in terms of its output value.
 * Recognizes the built-in activations; any other function is assumed to
 * have the sigmoid's derivative. */
static double genann_act_derivative(genann_actfun act, double y) {
    if (act == genann_act_tanh) return 1.0 - y * y;
    if (act == genann_act_relu) return y > 0 ? 1.0 : 0.0;
    if (act == genann_act_linear) return 1.0;
    return y * (1.0 - y);
}

/* Creates and returns a new ann. */
genann *genann_init(int inputs, int hidden_layers, int hidden, int outputs) {
    if (hidden_layers < 0) return 0;
    if (inputs < 1) return 0;
    if (outputs < 1) return 0;
    if (hidden_layers > 0 && hidden < 1) return 0;
    if (inputs > GENANN_MAX_DIMENSION || hidden_layers > GENANN_MAX_DIMENSION
            || hidden > GENANN_MAX_DIMENSION || outputs > GENANN_MAX_DIMENSION) return 0;


    const long long hidden_weights = hidden_layers ? (long long)(inputs+1) * hidden + (long long)(hidden_layers-1) * (hidden+1) * hidden : 0;
    const long long output_weights = (long long)(hidden_layers ? (hidden+1) : (inputs+1)) * outputs;
    const long long total_weights = (hidden_weights + output_weights);

    const long long total_neurons = ((long long)inputs + (long long)hidden * hidden_layers + outputs);

    /* Reject networks too large for the int counters and buffer size below. */
    if (total_weights > INT_MAX / 32 || total_neurons > INT_MAX / 32) return 0;

    /* Allocate extra size for weights, outputs, and deltas. */
    const int size = sizeof(genann) + sizeof(double) * (total_weights + total_neurons + (total_neurons - inputs));
    genann *ret = runtime.malloc(size);
    if (!ret) return 0;

    ret->inputs = inputs;
    ret->hidden_layers = hidden_layers;
    ret->hidden = hidden;
    ret->outputs = outputs;

    ret->total_weights = total_weights;
    ret->total_neurons = total_neurons;

    /* Set pointers. */
    ret->weight = (double*)((char*)ret + sizeof(genann));
    ret->output = ret->weight + ret->total_weights;
    ret->delta = ret->output + ret->total_neurons;

    genann_randomize(ret);

    ret->activation_hidden = genann_act_sigmoid_cached;
    ret->activation_output = genann_act_sigmoid_cached;

    genann_init_sigmoid_lookup(ret);

    return ret;
}

/* Creates ANN from file saved with genann_write. */
genann *genann_read(runtime.FILE *in) {
    int inputs, hidden_layers, hidden, outputs;
    int rc;

    runtime.errno = 0;
    rc = runtime.fscanf(in, "%d %d %d %d", &inputs, &hidden_layers, &hidden, &outputs);
    if (rc < 4 || runtime.errno != 0) {
        runtime.perror("fscanf");
        return 0;
    }

    genann *ann = genann_init(inputs, hidden_layers, hidden, outputs);
    if (!ann) return 0;

    int i;
    for (i = 0; i < ann->total_weights; ++i) {
        runtime.errno = 0;
        rc = runtime.fscanf(in, " %le", ann->weight + i);
        if (rc < 1 || runtime.errno != 0) {
            runtime.perror("fscanf");
            genann_free(ann);

            return 0;
        }
    }

    return ann;
}

/* Returns a new copy of ann. */
genann *genann_copy(genann const *ann) {
    const int size = sizeof(genann) + sizeof(double) * (ann->total_weights + ann->total_neurons + (ann->total_neurons - ann->inputs));
    genann *ret = runtime.malloc(size);
    if (!ret) return 0;

    runtime.memcpy(ret, ann, size);

    /* Set pointers. */
    ret->weight = (double*)((char*)ret + sizeof(genann));
    ret->output = ret->weight + ret->total_weights;
    ret->delta = ret->output + ret->total_neurons;

    return ret;
}

/* Frees the memory used by an ann. */
void genann_free(genann *ann) {
    /* The weight, output, and delta pointers go to the same buffer. */
    runtime.free(ann);
}

/* Runs the feedforward algorithm to calculate the ann's output. */
double const *genann_run(genann const *ann, double const *inputs) {
    double const *w = ann->weight;
    double *o = ann->output + ann->inputs;
    double const *i = ann->output;

    /* Copy the inputs to the scratch area, where we also store each neuron's
     * output, for consistency. This way the first layer isn't a special case. */
    runtime.memcpy(ann->output, inputs, sizeof(double) * ann->inputs);

    int h, j, k;

    if (!ann->hidden_layers) {
        double *ret = o;
        for (j = 0; j < ann->outputs; ++j) {
            double sum = *w++ * -1.0;
            for (k = 0; k < ann->inputs; ++k) {
                sum += *w++ * i[k];
            }
            *o++ = ann->activation_output(ann, sum);
        }

        return ret;
    }

    /* Figure input layer */
    for (j = 0; j < ann->hidden; ++j) {
        double sum = *w++ * -1.0;
        for (k = 0; k < ann->inputs; ++k) {
            sum += *w++ * i[k];
        }
        *o++ = ann->activation_hidden(ann, sum);
    }

    i += ann->inputs;

    /* Figure hidden layers, if any. */
    for (h = 1; h < ann->hidden_layers; ++h) {
        for (j = 0; j < ann->hidden; ++j) {
            double sum = *w++ * -1.0;
            for (k = 0; k < ann->hidden; ++k) {
                sum += *w++ * i[k];
            }
            *o++ = ann->activation_hidden(ann, sum);
        }

        i += ann->hidden;
    }

    double const *ret = o;

    /* Figure output layer. */
    for (j = 0; j < ann->outputs; ++j) {
        double sum = *w++ * -1.0;
        for (k = 0; k < ann->hidden; ++k) {
            sum += *w++ * i[k];
        }
        *o++ = ann->activation_output(ann, sum);
    }

    /* Sanity check that we used all weights and wrote all outputs. */
    if (w - ann->weight != ann->total_weights) runtime.abort();
    if (o - ann->output != ann->total_neurons) runtime.abort();

    return ret;
}

/* Does a single backprop update. */
void genann_train(genann const *ann, double const *inputs, double const *desired_outputs, double learning_rate) {
    /* To begin with, we must run the network forward. */
    genann_run(ann, inputs);

    int h, j, k;

    /* First set the output layer deltas. */
    {
        double const *o = ann->output + ann->inputs + ann->hidden * ann->hidden_layers; /* First output. */
        double *d = ann->delta + ann->hidden * ann->hidden_layers; /* First delta. */
        double const *t = desired_outputs; /* First desired output. */


        /* Set output layer deltas. */
        if (ann->activation_output == genann_act_linear) {
            for (j = 0; j < ann->outputs; ++j) {
                *d++ = *t++ - *o++;
            }
        } else {
            for (j = 0; j < ann->outputs; ++j) {
                *d++ = (*t - *o) * genann_act_derivative(ann->activation_output, *o);
                ++o; ++t;
            }
        }
    }


    /* Set hidden layer deltas, start on last layer and work backwards. */
    /* Note that loop is skipped in the case of hidden_layers == 0. */
    for (h = ann->hidden_layers - 1; h >= 0; --h) {

        /* Find first output and delta in this layer. */
        double const *o = ann->output + ann->inputs + (h * ann->hidden);
        double *d = ann->delta + (h * ann->hidden);

        /* Find first delta in following layer (which may be hidden or output). */
        double const * const dd = ann->delta + ((h+1) * ann->hidden);

        /* Find first weight in following layer (which may be hidden or output). */
        double const * const ww = ann->weight + ((ann->inputs+1) * ann->hidden) + ((ann->hidden+1) * ann->hidden * (h));

        for (j = 0; j < ann->hidden; ++j) {

            double delta = 0;

            for (k = 0; k < (h == ann->hidden_layers-1 ? ann->outputs : ann->hidden); ++k) {
                const double forward_delta = dd[k];
                const int windex = k * (ann->hidden + 1) + (j + 1);
                const double forward_weight = ww[windex];
                delta += forward_delta * forward_weight;
            }

            *d = genann_act_derivative(ann->activation_hidden, *o) * delta;
            ++d; ++o;
        }
    }


    /* Train the outputs. */
    {
        /* Find first output delta. */
        double const *d = ann->delta + ann->hidden * ann->hidden_layers; /* First output delta. */

        /* Find first weight to first output delta. */
        double *w = ann->weight + (ann->hidden_layers
                ? ((ann->inputs+1) * ann->hidden + (ann->hidden+1) * ann->hidden * (ann->hidden_layers-1))
                : (0));

        /* Find first input in previous layer. */
        double const * const i = ann->output + (ann->hidden_layers
                ? (ann->inputs + (ann->hidden) * (ann->hidden_layers-1))
                : 0);

        /* Set output layer weights. */
        for (j = 0; j < ann->outputs; ++j) {
            *w++ += *d * learning_rate * -1.0;
            for (k = 1; k < (ann->hidden_layers ? ann->hidden : ann->inputs) + 1; ++k) {
                *w++ += *d * learning_rate * i[k-1];
            }

            ++d;
        }

        if (w - ann->weight != ann->total_weights) runtime.abort();
    }


    /* Train the hidden layers. */
    for (h = ann->hidden_layers - 1; h >= 0; --h) {

        /* Find first delta in this layer. */
        const double *d = ann->delta + (h * ann->hidden);

        /* Find first input to this layer. */
        const double *i = ann->output + (h
                ? (ann->inputs + ann->hidden * (h-1))
                : 0);

        /* Find first weight to this layer. */
        double *w = ann->weight + (h
                ? ((ann->inputs+1) * ann->hidden + (ann->hidden+1) * (ann->hidden) * (h-1))
                : 0);


        for (j = 0; j < ann->hidden; ++j) {
            *w++ += *d * learning_rate * -1.0;
            for (k = 1; k < (h == 0 ? ann->inputs : ann->hidden) + 1; ++k) {
                *w++ += *d * learning_rate * i[k-1];
            }
            ++d;
        }
    }
}

/* Saves the ann. */
void genann_write(genann const *ann, runtime.FILE *out) {
    runtime.fprintf(out, "%d %d %d %d", ann->inputs, ann->hidden_layers, ann->hidden, ann->outputs);

    int i;
    for (i = 0; i < ann->total_weights; ++i) {
        runtime.fprintf(out, " %.20e", ann->weight[i]);
    }
}