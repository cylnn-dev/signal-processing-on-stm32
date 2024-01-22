//
// Created by CylNn on 24.12.2023.
//
#include "freertos_util.h"
#include <stdint-gcc.h>
#include <stddef.h>

void clear_prev_positions(char *message, uint32_t index, uint32_t offset) {
    for (size_t i = index; i < index + offset; ++i) {
        message[i] = ' ';
    }
}