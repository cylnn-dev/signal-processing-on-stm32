//
// Created by CylNn on 24.12.2023.
//

#include <stdint-gcc.h>
#include <stddef.h>
#include <string.h>
#include "filter_util.h"
#include "tinyusb_utils.h"
#include "filters.h"
#include "SEGGER_SYSVIEW.h"

Filter filters[MAX_FILTER_HOLD] = {0};
SOSFilter sos_filters[MAX_FILTER_HOLD] = {0};
size_t current_filter_index = -1;
size_t current_sos_index = -1;
size_t max_filter_index = 0;
size_t max_sos_index = 0;
extern bool use_sos;


SignalRecordBuffer filter_buffer = {{0}, {0}, {0}, {0}, {0}, {0}};


extern SOSFilter iir_but_1500;
extern Filter fir_equ_high_7k_10k;
extern Filter iir_but_low_1500_7k;
extern Filter iir_eli_bpass_4k_6k;
extern Filter iir_eli_bpass_4k_6k_direct2;
extern Filter fir_taylor_bstop_10k;

//FilterCommonType current_filter;
Filter current_filter = {0};
SOSFilter current_sos_filter = {0};

void init_filter_array(void) {
    filters[++current_filter_index] = fir_equ_high_7k_10k;
    filters[++current_filter_index] = iir_but_low_1500_7k;
    filters[++current_filter_index] = iir_eli_bpass_4k_6k;
    filters[++current_filter_index] = iir_eli_bpass_4k_6k_direct2;
    filters[++current_filter_index] = fir_taylor_bstop_10k;
    /* add other filters here */
    max_filter_index = current_filter_index;
    current_filter = filters[current_filter_index];

    sos_filters[++current_sos_index] = iir_but_1500;
    /* add other filters here */
    max_sos_index = current_sos_index;
    current_sos_filter = sos_filters[current_filter_index];
}

void cycle_over(void) {
    if (use_sos) {
        if (current_sos_index < max_sos_index) current_sos_index += 1;
        else current_sos_index = 0;
    } else {
        if (current_filter_index < max_filter_index) current_filter_index += 1;
        else current_filter_index = 0;
    }
    current_filter = filters[current_filter_index];
    current_sos_filter = sos_filters[current_sos_index];
    flush_filter_buffer();
}

char * get_current_filter_name(void) {
    if (use_sos) {
        return current_sos_filter.name;
    } else {
        return current_filter.name;
    }
}

void flush_filter_buffer(void) {
    memset(filter_buffer.current_outputs, 0, SIGNAL_SIZE_BYTES);
    memset(filter_buffer.current_inputs, 0, SIGNAL_SIZE_BYTES);
    memset(filter_buffer.current_w_values, 0, SIGNAL_SIZE_BYTES);
    memset(filter_buffer.past_inputs, 0, SIGNAL_SIZE_BYTES);
    memset(filter_buffer.past_outputs, 0, SIGNAL_SIZE_BYTES);
    memset(filter_buffer.past_w_values, 0, SIGNAL_SIZE_BYTES);
}

void fir_filter(int16_t *output) {
    for (int i = 0; i < SIGNAL_SIZE; ++i) {
        int sample = 0;

        for (int j = 0; j < current_filter.filter_size; ++j) {
            int index = i - j;
            if (index < 0) {
                index += SIGNAL_SIZE;
                sample += current_filter.numerators[j] * filter_buffer.past_inputs[index];
            } else {
                sample += current_filter.numerators[j] * filter_buffer.current_inputs[index];
            }
        }
        filter_buffer.current_outputs[i] = (int16_t) (sample >> current_filter.fraction_len);
    }

    memcpy(output, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
//    memcpy(filter_buffer.past_outputs, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
    memcpy(filter_buffer.past_inputs, filter_buffer.current_inputs, SIGNAL_SIZE_BYTES);
}

void iir_filter_direct_1(int16_t *output) {
    for (int i = 0; i < SIGNAL_SIZE; ++i) {
        int32_t sample = 0;

        /* b coefficients are here */
        for (int j = 0; j < current_filter.filter_size; ++j) {
            int index = i - j;
            if (index < 0) {
                index += SIGNAL_SIZE;
                sample += current_filter.numerators[j] * filter_buffer.past_inputs[index];
            } else {
                sample += current_filter.numerators[j] * filter_buffer.current_inputs[index];
            }
        }

        /* a coefficients are here */
        for (int j = 1; j < current_filter.filter_size; ++j) {
            int index = i - j;
            if (index < 0) {
                index += SIGNAL_SIZE;
                sample -= current_filter.denominators[j] * filter_buffer.past_outputs[index];
            } else {
                sample -= current_filter.denominators[j] * filter_buffer.current_outputs[index];
            }
        }

        filter_buffer.current_outputs[i] = (int16_t) (sample >> current_filter.fraction_len);
    }

    memcpy(output, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
    memcpy(filter_buffer.past_outputs, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
    memcpy(filter_buffer.past_inputs, filter_buffer.current_inputs, SIGNAL_SIZE_BYTES);
}


void iir_filter_direct_2(int16_t *output) {
    for (int i = 0; i < SIGNAL_SIZE; ++i) {
        int32_t current_w_value = filter_buffer.current_inputs[i];

        /* a */
        for (int j = 1; j < current_filter.filter_size; ++j) {
            int index = i - j;
            if (index < 0) {
                index += SIGNAL_SIZE;
                current_w_value -= current_filter.denominators[j] * filter_buffer.past_w_values[index];
            } else {
                current_w_value -= current_filter.denominators[j] * filter_buffer.current_w_values[index];
            }
        }
        filter_buffer.current_w_values[i] = (int16_t) (current_w_value >> current_filter.fraction_len);

        /* b */
        int64_t sample = 0;
        for (int j = 0; j < current_filter.filter_size; ++j) {
            int index = i - j;
            if (index < 0) {
                index += SIGNAL_SIZE;
                sample += current_filter.numerators[j] * filter_buffer.past_w_values[index];
            } else {
                sample += current_filter.numerators[j] * filter_buffer.current_w_values[index];
            }
        }

        filter_buffer.current_outputs[i] = (int16_t) (sample);
    }

    memcpy(output, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
    memcpy(filter_buffer.past_outputs, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
    memcpy(filter_buffer.past_inputs, filter_buffer.current_inputs, SIGNAL_SIZE_BYTES);
    memcpy(filter_buffer.past_w_values, filter_buffer.current_w_values, SIGNAL_SIZE_BYTES);
}


void iir_filter_sos_direct_1(int16_t *output) {
    for (int section = 0; section < current_sos_filter.n_section; ++section) {
        for (int i = 0; i < SIGNAL_SIZE; ++i) {
            int32_t  sample = 0;

            /* b coefficients are here */
            for (int j = 0; j < current_filter.filter_size; ++j) {
                int index = i - j;
                if (index < 0) {
                    index += SIGNAL_SIZE;
                    sample += current_sos_filter.numerators[section][j] * filter_buffer.past_inputs[index];
                } else {
                    sample += current_sos_filter.numerators[section][j] * filter_buffer.current_inputs[index];
                }
            }

            /* a coefficients are here */
            for (int j = 1; j < current_filter.filter_size; ++j) {
                int index = i - j;
                if (index < 0) {
                    index += SIGNAL_SIZE;
                    sample -= current_sos_filter.denominators[section][j] * filter_buffer.past_outputs[index];
                } else {
                    sample -= current_sos_filter.denominators[section][j] * filter_buffer.current_outputs[index];
                }
            }

            filter_buffer.current_outputs[i] = (int16_t) (sample >> current_filter.fraction_len);
        }
//        memcpy(output, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
        memcpy(filter_buffer.past_outputs, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
        memcpy(filter_buffer.past_inputs, filter_buffer.current_inputs, SIGNAL_SIZE_BYTES);
    }

//    memcpy(output, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
//    memcpy(filter_buffer.past_outputs, filter_buffer.current_outputs, SIGNAL_SIZE_BYTES);
    memcpy(filter_buffer.past_inputs, filter_buffer.current_inputs, SIGNAL_SIZE_BYTES);
}

//void iir_filter_direct_1(int16_t *output) {
//    for (int i = 0; i < SIGNAL_SIZE; ++i) {
//        int sample = 0;
//
//        for (int j = 0; j < current_filter.filter_size; ++j) {
//            int index = i - j;
//            if (index < 0) {
//                index += SIGNAL_SIZE;
//                sample += (int) current_filter.numerators[j] * filter_buffer.inputs[index];
//            } else {
//                sample += (int) current_filter.numerators[j] * filter_buffer.current_inputs[index];
//            }
//        }
//
//        for (int j = 1; j < current_filter.filter_size; ++j) {
//            int index = i + j;
//            if (index >= SIGNAL_SIZE) {
//                index -= SIGNAL_SIZE;
//                sample -= (int) current_filter.denominators[j] * filter_buffer.past_outputs[index];
//            } else {
//                sample -= (int) current_filter.denominators[j] * filter_buffer.current_inputs[index];
//            }
//        }
//
//        // Uncomment if required for scaing
////         sample /= current_filter.denominators[0];
//
//        filter_buffer.current_inputs[i] = (int16_t)(sample >> 15);
//    }
//
//    memcpy(output, filter_buffer.current_inputs, SIGNAL_SIZE_BYTES);
//    memcpy(filter_buffer.inputs, filter_buffer.current_inputs, SIGNAL_SIZE_BYTES);
//    memcpy(filter_buffer.current_inputs, filter_buffer.past_outputs, SIGNAL_SIZE_BYTES);
//}



//void fir_filter(int16_t *output) {
//    int current_head = filter_buffer.current_head == FIRST_HALF ? (0) : (SIGNAL_SIZE);
//    for (int i = 0; i < SIGNAL_SIZE; ++i) {
//        int sample = 0;
//
//        for (int j = 0; j < current_filter.filter_size; ++j) {
//            int in_index = current_head + i - j;
//            if (in_index < 0) in_index += SIGNAL_SIZE_2;
//            sample += current_filter.numerators[j] * filter_buffer.data[in_index];
//        }
//
//        output[i] = (int16_t) (sample >> 15);
//    }
////    TOGGLE_BUFFER_STATE(filter_buffer);
//    filter_buffer.current_head = (filter_buffer.current_head == FIRST_HALF) ? SECOND_HALF : FIRST_HALF;
//}


