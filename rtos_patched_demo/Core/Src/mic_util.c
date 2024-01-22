//
// Created by CylNn on 3.01.2024.
//

#include "mic_util.h"
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

extern uint16_t mic_pdm_buffer[SIGNAL_SIZE * 2];
extern TaskHandle_t mic_taskHandle;
BufferStates mic_pdm_buffer_state = FIRST_HALF;

void enable_mic_dma(void) { HAL_I2S_Receive_DMA(&hi2s2,  mic_pdm_buffer, SIGNAL_SIZE * 2); }


float InterpolateHermite(float x0, float x1, float x2, float x3, float t)
{
    float diff = x1 - x2;
    float c1 = x2 - x0;
    float c3 = x3 - x0 + 3 * diff;
    float c2 = -(2 * diff + c1 + c3);
    return 0.5f * ((c3 * t + c2) * t + c1) * t + x1;
}

void interpolate(void) {

}



void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == SPI2) {
        portBASE_TYPE pxHigherPriorityTaskWoken = pdFALSE;
        SEGGER_SYSVIEW_RecordEnterISR();

        mic_pdm_buffer_state = FIRST_HALF;
        vTaskNotifyGiveFromISR(mic_taskHandle, &pxHigherPriorityTaskWoken);

        SEGGER_SYSVIEW_RecordExitISR();
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken)
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == SPI2) {
        portBASE_TYPE pxHigherPriorityTaskWoken = pdFALSE;
        SEGGER_SYSVIEW_RecordEnterISR();

        mic_pdm_buffer_state = SECOND_HALF;
        vTaskNotifyGiveFromISR(mic_taskHandle, &pxHigherPriorityTaskWoken);

        SEGGER_SYSVIEW_RecordExitISR();
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken)
    }
}
