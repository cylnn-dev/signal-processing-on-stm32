//
// Created by CylNn on 24.12.2023.
//

#ifndef RTOS_PATCHED_DEMO_FREERTOS_UTIL_H
#define RTOS_PATCHED_DEMO_FREERTOS_UTIL_H

/********* SIGNAL GENERATION ********/
#include <stdint-gcc.h>

#define SAMPLE_FREQ 48000
#define SAMPLE_PERIOD (1.0f / SAMPLE_FREQ)
#define N_POINTS (480 * 2)
#define MAX_FREQS 20

typedef enum {
    MIC_ENABLE      = (1 << 0),
    SIGNAL_ENABLE   = (1 << 1),
    ADC_ENABLE      = (1 << 2),
    USB_ENABLE      = (1 << 3),
    DAC_ENABLE      = (1 << 4),
    DRIVER_ENABLE   = (1 << 5),
    FILTER_ENABLE   = (1 << 6),
    NONE            = 0
} ModeState;

typedef enum {
    DATA_REQUEST        = 0x00U,
    FIRST_HALF_READY    = 0x01U,
    SECOND_HALF_READY   = 0x02U,
    BUFFER_FULL         = 0x03U,
    BUFFER_ERROR        = 0x04U,
} BufferState;


typedef enum {
    COS         = 0,
    SAW         = 1,
    TRI         = 2,
    SQU         = 3,
    IMP         = 4,
} SignalTypes;

typedef struct {
    SignalTypes signal_type;
    int16_t freqs[MAX_FREQS];
    int amplitude;
    float sample_period;
    int dc_point;
    int duty_cycle;
} SignalSettings;

typedef enum {
    MAIN_MENU   = 0,
    MODE_MENU   = 1,
    SIGNAL_MENU   = 2,
    FILTER_MENU = 3,
} Program_States;

typedef void(*StateFunction)(char);

void clear_prev_positions(char *message, uint32_t index, uint32_t offset);

#endif //RTOS_PATCHED_DEMO_FREERTOS_UTIL_H
