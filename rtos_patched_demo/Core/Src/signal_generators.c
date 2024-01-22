//
// Created by CylNn on 24.12.2023.
//

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include <string.h>
#include "signal_generators.h"
#include "tinyusb_utils.h"
#include "freertos_util.h"
#include <math.h>
#include <stdint-gcc.h>
#include <stdlib.h>
#include "usart.h"
#include "portmacro.h"
#include "i2s.h"
#include "filter_util.h"


#define DEFINED_DC_RESOLUTION 10
#define MAX_DC 1000
static int pre_defined_dc_points[DEFINED_DC_RESOLUTION + 1] = {0};

#define DEFINED_AMP_RESOLUTION 10
#define MAX_AMPLITUDE 20
static int pre_defined_amplitudes[DEFINED_AMP_RESOLUTION + 1] = {0};
int amp_index = 0;
int dc_index = 0;

static int16_t cos_lut[N_POINTS] = {0};
int lut_top = 0;
int period_index = 0;
int16_t min_freq = INT16_MAX;   // this value will be updated at the end of calculate_lut()

extern int16_t signal_buffer[SIGNAL_SIZE];

extern StreamBufferHandle_t usb_bufferHandle;
extern SignalSettings signal_settings;


int get_next_amplitude(void) {
    if (++amp_index > DEFINED_AMP_RESOLUTION) amp_index = 0;
    return pre_defined_amplitudes[amp_index];
}

int get_next_dc_point(void) {
    if(++dc_index > DEFINED_DC_RESOLUTION) dc_index = 0;
    return pre_defined_dc_points[dc_index];
}

void calculate_dc_points(void) {
    int step = (int) (2 * MAX_DC / DEFINED_DC_RESOLUTION);
    int current_amplitude = - MAX_DC;
    for (int i = 0; i < DEFINED_DC_RESOLUTION + 1; ++i) {
        pre_defined_dc_points[i] = current_amplitude;
        current_amplitude += (step);
    }
}

void calculate_amplitudes(void) {
    int step = (int16_t) MAX_AMPLITUDE / DEFINED_AMP_RESOLUTION;
    int current_amplitude = 1;
    for (int i = 0; i < DEFINED_AMP_RESOLUTION; ++i) {
        pre_defined_amplitudes[i] = current_amplitude;

        if (current_amplitude == 1) current_amplitude += step - 1;
        else current_amplitude += step;
    }
}

void update_min_freq(void) {
    for (int i = 0; i < MAX_FREQS; ++i) {
        int16_t freq = signal_settings.freqs[i];
        if (freq < 1) break;
        if (freq < min_freq) min_freq = freq;
    }
    assert_param(min_freq != INT16_MAX); // cannot find the min_freq
}

void calculate_lut(void) {
    float t = 0;
    for (int i = 0; i < N_POINTS; ++i) {
        float f_sample = 0;
        for (int j = 0; j < MAX_FREQS; ++j) {
            int16_t freq = signal_settings.freqs[j];
            if (freq < 1) break;
            f_sample += cosf(2 * M_PI * freq * t);
        }
        cos_lut[i] = (int16_t) (((float) signal_settings.amplitude * f_sample + signal_settings.dc_point) * 1000);
        t += signal_settings.sample_period;
    }
    update_min_freq();

    /* lastly update period_index here too */
    period_index = (int16_t) (SAMPLE_FREQ / min_freq);
}

void generate_cos(void) {
    memcpy(signal_buffer, &cos_lut[lut_top], SIGNAL_SIZE_BYTES);
    lut_top += SIGNAL_SIZE;
    if (lut_top >= period_index) lut_top = 0;

}

void generate_saw(void) {
    int cycle_length = SIGNAL_SIZE * signal_settings.duty_cycle;

    for (int i = 0; i < SIGNAL_SIZE; ++i) {
        signal_buffer[i] = (int16_t)((i % cycle_length) * (1000 / SIGNAL_SIZE));
    }
}

void generate_triangle(void) {
    /* very simple version of triangle waveform */

    for (int i = 0; i < SIGNAL_SIZE; ++i) {
        signal_buffer[i] = abs((i % 6) - 3);
    }
}

void generate_square(void) {
    /* very simple version of square waveform */

    for (int i = 0; i < SIGNAL_SIZE; ++i) {
        signal_buffer[i] = (i % 6) < 3 ? 3 : 0;
    }

}

void generate_impulse(void) {
    for (int i = 0; i < SIGNAL_SIZE; ++i) {
        signal_buffer[i] = (int16_t) signal_settings.amplitude;
    }
}


