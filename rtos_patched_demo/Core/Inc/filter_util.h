//
// Created by CylNn on 24.12.2023.
//

#ifndef RTOS_PATCHED_DEMO_FILTER_UTIL_H
#define RTOS_PATCHED_DEMO_FILTER_UTIL_H

#include <stdbool.h>
#include "tinyusb_utils.h"
#include <stdint-gcc.h>

#define MAX_COEF_SIZE 96
#define MAX_FILTER_NAME 64
#define MAX_FILTER_HOLD 5
#define MAX_SECTION_SIZE 50
#define SIGNAL_SIZE_2 (2 * SIGNAL_SIZE)

typedef enum {
    FIR = 0,
    IIR_DIRECT1 = 1,
    IIR_DIRECT2 = 2,
    IIR_T_DIRECT2 = 3
} FilterTypes;

typedef struct {
    char name[MAX_FILTER_NAME];
    int filter_size;
    int fraction_len;
    FilterTypes filter_type;
    int16_t denominators[MAX_COEF_SIZE];
    int16_t numerators[MAX_COEF_SIZE];
} Filter;

typedef struct {
    char name[MAX_FILTER_NAME];
    int n_section;
    int fraction_len;
    FilterTypes filter_type;
    int numerator_lengths[MAX_SECTION_SIZE];
    int denominator_lengths[MAX_SECTION_SIZE];
    int16_t denominators[MAX_SECTION_SIZE][3];
    int16_t numerators[MAX_SECTION_SIZE][3];
} SOSFilter; // second order section type

typedef union {
    Filter basic_filter;
    SOSFilter sos_filter;
} FilterType;

typedef struct {
    enum {is_basic, is_sos} type;
    FilterType filter;
} FilterCommonType;

typedef enum {
    FIRST_HALF = 0,
    SECOND_HALF = SIGNAL_SIZE,
} BufferStates;

#define TOGGLE_BUFFER_STATE(buffer) (buffer.current_head ^= SIGNAL_SIZE)


typedef struct {
    int16_t current_inputs[SIGNAL_SIZE];
    int16_t past_inputs[SIGNAL_SIZE];
    int16_t current_outputs[SIGNAL_SIZE];
    int16_t past_outputs[SIGNAL_SIZE];
    // State variables for direct form II
    int16_t current_w_values[SIGNAL_SIZE];
    int16_t past_w_values[SIGNAL_SIZE];
} SignalRecordBuffer;

typedef void(*FilterFunction)(int16_t *output);

void init_filter_array(void);

void fir_filter(int16_t *output);
void iir_filter_direct_1(int16_t *output);
void iir_filter_direct_2(int16_t *output);
void iir_filter_transposed_direct_2(int16_t *output);
void iir_filter_sos_direct_1(int16_t *output);
void cycle_over(void);
char * get_current_filter_name(void);
void flush_filter_buffer(void);

#endif //RTOS_PATCHED_DEMO_FILTER_UTIL_H
