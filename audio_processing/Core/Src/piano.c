//
// Created by CylNn on 17.11.2023.
//

#include "piano.h"

#define CHAR_FREQ_SIZE 12

struct char_freq {
    char character;
    float freq;
};

const struct char_freq char2freq_lookup[CHAR_FREQ_SIZE] = {
        {'z', 261.63f},
        {'s', 277.18f},
        {'x', 293.66f},
        {'d', 311.13f},
        {'c', 329.63f},
        {'v', 349.23f},
        {'g', 369.99f},
        {'b', 392.00f},
        {'h', 415.30f},
        {'n', 440.00f},
        {'j', 466.16f},
        {'m', 493.88f},
};

float get_frequency(char c) {
    for (int i = 0; i < CHAR_FREQ_SIZE; ++i) {
        if (char2freq_lookup[i].character == c) {
            return char2freq_lookup[i].freq;
        }
    }
    return 0.0f; // value not found
}
