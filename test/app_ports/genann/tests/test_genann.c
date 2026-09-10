// expect: 0
package main;

import genann;
import runtime;

extern double exp(double x);
extern double tanh(double x);
extern int rand(void);

int main() {
    // Create a simple neural network: 2 inputs, 1 hidden layer with 3 neurons, 1 output
    genann.genann *ann = genann.genann_init(2, 1, 3, 1);
    if (!ann) {
        runtime.printf("Failed to initialize network\n");
        return 1;
    }

    // Set fixed weights for predictable output
    double fixed_weights[] = {
        // Input to hidden layer weights
        -0.5, 0.3, -0.2,  // Neuron 0
        0.1, -0.4, 0.2,   // Neuron 1
        -0.3, 0.5, -0.1,  // Neuron 2
        // Hidden to output layer weights
        0.2, -0.3, 0.1, 0.4
    };

    // Verify weight count matches
    if (sizeof(fixed_weights)/sizeof(double) != ann->total_weights) {
        runtime.printf("Weight count mismatch: expected %d, got %zu\n",
                       ann->total_weights, sizeof(fixed_weights)/sizeof(double));
        genann.genann_free(ann);
        return 1;
    }

    // Copy fixed weights
    for (int i = 0; i < ann->total_weights; i++) {
        ann->weight[i] = fixed_weights[i];
    }

    // Test input: [1.0, 0.0]
    double input[] = {1.0, 0.0};

    // Run the network
    double const *output = genann.genann_run(ann, input);

    // With the fixed weights above and input [1.0, 0.0], the network output
    // is sigmoid(-0.093...) ~= 0.4769. Verify it is a valid sigmoid value in
    // (0,1) and close to that analytically-derived value.
    if (!(output[0] > 0.0 && output[0] < 1.0)) {
        runtime.printf("Test failed: output out of range, got %f\n", output[0]);
        genann.genann_free(ann);
        return 1;
    }
    if (output[0] < 0.45 || output[0] > 0.50) {
        runtime.printf("Test failed: unexpected output, got %f\n", output[0]);
        genann.genann_free(ann);
        return 1;
    }

    // Print output for verification
    runtime.printf("Output: %f\n", output[0]);

    // Clean up
    genann.genann_free(ann);
    return 0;
}