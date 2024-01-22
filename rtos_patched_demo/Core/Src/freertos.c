/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "tim.h"
#include "stream_buffer.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "device/usbd.h"
#include "tinyusb_utils.h"
#include "freertos_util.h"
#include "signal_generators.h"
#include "filter_util.h"
#include "mic_util.h"
#include "pdm2pcm.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
TaskHandle_t init_taskHandle = NULL;
TaskHandle_t uart_rx_taskHandle = NULL;
TaskHandle_t uart_tx_taskHandle = NULL;
TaskHandle_t led_ctrl_taskHandle = NULL;
TaskHandle_t generate_signal_taskHandle = NULL;
TaskHandle_t usb_taskHandle = NULL;
TaskHandle_t filter_taskHandle = NULL;
TaskHandle_t mic_taskHandle = NULL;

StreamBufferHandle_t uart_tx_bufferHandle = NULL;   // this is the stream buffer
//StaticStreamBuffer_t uart_tx_streamHandle;          // this is the handle to assert for stream buffer
StreamBufferHandle_t usb_bufferHandle = NULL;   // this is the stream buffer


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

extern uint8_t uart_tx_buffer[UART_TX_BUFFER_SIZE + 1];
extern uint8_t uart_rx_char;

int16_t signal_buffer[SIGNAL_SIZE] = {0};
uint16_t mic_pdm_buffer[SIGNAL_SIZE * 2] = {0};
extern BufferStates mic_pdm_buffer_state;
extern SignalRecordBuffer filter_buffer;

/** initials variables and menu settings */
Program_States program_state = MAIN_MENU;
ModeState mode_states = NONE;
SignalFunction signal_functions[] = {generate_cos, generate_saw, generate_triangle, generate_square, generate_impulse};
FilterFunction filter_functions[] = {fir_filter, iir_filter_direct_1, iir_filter_direct_2};

SignalSettings signal_settings = {
        .signal_type = COS,
        .freqs = {100, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 7000, 10000, 15000},
        .amplitude = 1,
        .sample_period = 1.0f / 48000.0f,
        .dc_point = 0,
        .duty_cycle = 5
};

bool use_sos = false;

extern Filter current_filter;

static inline const char *enum2string(SignalTypes signal_type) {
    static const char *values[] = {"COS", "SAW", "TRI", "SQU", "IMP"};

    return values[signal_type];
}

static inline const char *freq2string(int16_t *freqs) {
    static char freq_str[MAX_FREQS * 10];
    int chars_written = 0;
    int offset = 0;

    for (int i = 0; i < MAX_FREQS; ++i) {
        int16_t freq = freqs[i];
        if (freq < 1) break;
        chars_written = snprintf(freq_str + offset, sizeof(freq_str) - offset, "%d, ", freq);
        if (chars_written < 0) {
            SEGGER_SYSVIEW_PrintfHost("error on writing chars");
            return NULL;
        }
        offset += chars_written;
    }
    return freq_str;
}

static inline const char *int2str(int amplitude) {
    static char amp_str[6];
//    memset(amp_str, 0, sizeof(amp_str));
    int chars_written = snprintf(amp_str, sizeof(amp_str), "%d", (int) amplitude);
    if (chars_written < 0) {
        // something is wrong on amp_str
        SEGGER_SYSVIEW_PrintfHost("error on writing chars");
        return NULL;
    }
    return amp_str;
}
/******************************/

#define MIC_IN_SIZE (196)
#define PCM_SIZE    (MIC_IN_SIZE / 2)

BufferState mic_in_state = DATA_REQUEST;
BufferState pcm_state = DATA_REQUEST;

uint16_t mic_in_buffer[MIC_IN_SIZE] = {0};
int16_t pcm_buffer[PCM_SIZE] = {0};

/*** function prototypes ***/
void main_menu(const char input);
void mode_menu(const char input);
void signal_menu(const char input);
void filter_menu(const char input);
void update_mode_menu(void);
void update_signal_menu(void);
void update_filter_menu(void);

void apply_filter(void);

/* tasks */
void init_task(void *argument);
void input_control(void *argument);
void led_control(void *argument);
void uart_transmit_task(void *argument);
void generate_signal(void *argument);
void usb_task(void *argument);
void filter_task(void *argument);
void mic_task(void *argument);

StateFunction state_functions[] = {main_menu, mode_menu, signal_menu, filter_menu};
/***************************/

void update_mode_menu(void) {
    for (int i = 0; i < MENU_POSITIONS_SIZE; ++i) {
        mode_menu_message[mode_menu_positions[i]] = (mode_states & (1 << i)) ? ('x') : ('-');
    }
    uart_transmit(clear_code);
    uart_transmit(mode_menu_message);
}

void main_menu(const char input) {
    SEGGER_SYSVIEW_PrintfHost("main_menu running");
    switch (input) {
        case '1':
            program_state = MODE_MENU;
            uart_transmit(clear_code);
            uart_transmit(mode_menu_message);
            break;
        case '2':
            program_state = SIGNAL_MENU;
            update_signal_menu();
            break;
        case '3':
            program_state = FILTER_MENU;
            update_filter_menu();
            break;
        case '9':
            uart_transmit(clear_code);
            uart_transmit(about_message);
            break;
        case '0':
            program_state = MAIN_MENU;
            uart_transmit(clear_code);
            uart_transmit(main_menu_message);
            break;
        default:
            uart_transmit(not_understood_message);
            break;
    }
    uart_receive();
}

void mode_menu(const char input) {
    SEGGER_SYSVIEW_PrintfHost("mode_menu running");
    switch (input) {
        case '1':
            mode_states ^= MIC_ENABLE;
            mode_states &= ~(SIGNAL_ENABLE | ADC_ENABLE);
            break;
        case '2':
            mode_states ^= SIGNAL_ENABLE;
            mode_states &= ~(MIC_ENABLE | ADC_ENABLE);
            break;
        case '3':
            mode_states ^= ADC_ENABLE;
            mode_states &= ~(MIC_ENABLE | SIGNAL_ENABLE);
            break;
        case '4':
            mode_states ^= USB_ENABLE;
            break;
        case '5':
            mode_states ^= DAC_ENABLE;
            break;
        case '6':
            mode_states ^= DRIVER_ENABLE;
            break;
        case '7':
            mode_states ^= FILTER_ENABLE;
            break;
        case '0':
            program_state = MAIN_MENU;
            uart_transmit(clear_code);
            uart_transmit(main_menu_message);
            break;
        default:
            uart_transmit(not_understood_message);
            break;
    }
//    if (mode_states & MIC_ENABLE) {
//        vTaskSuspend(generate_signal_taskHandle);
//        vTaskResume(mic_taskHandle);
//    } else if (mode_states & SIGNAL_ENABLE) {
//        vTaskSuspend(mic_taskHandle);
//        vTaskResume(generate_signal_taskHandle);
//    }

    uart_receive();
    /* at the end of the mode selection, check all led states with the new modes */
    update_mode_menu();
    xTaskNotifyGive(led_ctrl_taskHandle);
}


void update_signal_menu(void) {
    const char *signal_type_str = enum2string(signal_settings.signal_type);
    memcpy(signal_menu_message + signal_menu_positions[0], signal_type_str, strlen(signal_type_str));

    const char *signal_freq_str = freq2string(signal_settings.freqs);
    memcpy(signal_menu_message + signal_menu_positions[1], signal_freq_str, strlen(signal_freq_str));

    clear_prev_positions(signal_menu_message, signal_menu_positions[2], 4);
    const char *signal_amp_str = int2str((int) signal_settings.amplitude);
    memcpy(signal_menu_message + signal_menu_positions[2], signal_amp_str, strlen(signal_amp_str));

    clear_prev_positions(signal_menu_message, signal_menu_positions[4], 5);
    const char *signal_dc_str = int2str((int) signal_settings.dc_point);
    memcpy(signal_menu_message + signal_menu_positions[4], signal_dc_str, strlen(signal_dc_str));


    uart_transmit(clear_code);
    uart_transmit(signal_menu_message);
}

void update_filter_menu(void) {
    const char *filter_type_str = (use_sos) ? ("SOS"): ("DIR");
    clear_prev_positions(filter_menu_message, filter_menu_positions[0], 3);
    memcpy(filter_menu_message + filter_menu_positions[0], filter_type_str, strlen(filter_type_str));

    clear_prev_positions(filter_menu_message, filter_menu_positions[1], 32);
    const char *filter_name_str = get_current_filter_name();
    memcpy(filter_menu_message + filter_menu_positions[1], filter_name_str, strlen(filter_name_str));

    uart_transmit(clear_code);
    uart_transmit(filter_menu_message);
}

void signal_menu(const char input) {
    SEGGER_SYSVIEW_PrintfHost("signal_menu running");
    switch (input) {
        case '1':
            signal_settings.signal_type = (signal_settings.signal_type == IMP) ? (COS) : (signal_settings.signal_type + 1);
            xStreamBufferReset(usb_bufferHandle);
            calculate_lut();
            break;
        case '2':
            // todo: find a way to change frequencies
            __NOP();
            break;
        case '3':
            signal_settings.amplitude = get_next_amplitude();
            SEGGER_SYSVIEW_PrintfHost("amplitude: %d", signal_settings.amplitude);
            calculate_lut();
            break;
        case '4':
            // todo: changing sample frequency is not supported
            __NOP();
            break;
        case '5':
            signal_settings.dc_point = get_next_dc_point();
            calculate_lut();
            break;
        case '9':
            break;
        case '0':
            program_state = MAIN_MENU;
            uart_transmit(clear_code);
            uart_transmit(main_menu_message);
            break;
        default:
            uart_transmit(not_understood_message);
            break;
    }
    update_signal_menu();
    uart_receive();
}

void filter_menu(const char input) {
    SEGGER_SYSVIEW_PrintfHost("filter_menu running");
    switch (input) {
        case '1':
            use_sos = !use_sos;
            break;
        case '2':
            cycle_over();
            break;
        case '0':
            program_state = MAIN_MENU;
            uart_transmit(clear_code);
            uart_transmit(main_menu_message);
            break;
        default:
            uart_transmit(not_understood_message);
            break;
    }
    update_filter_menu();
    uart_receive();
}

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */


/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
    UNUSED(xTask);
    UNUSED(pcTaskName);
   while (1) {
       //stackoverflow
       __NOP();
   }
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
    while (1) {
        // malloc failed
        __NOP();
    }
}
/* USER CODE END 5 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */

  BaseType_t uart_rx_task_xReturned = xTaskCreate(input_control,
                                                  "uartRxTask",
                                                  configMINIMAL_STACK_SIZE,
                                                  (void *) 1,
                                                  osPriorityBelowNormal,
                                                  &uart_rx_taskHandle);

  assert_param(uart_rx_task_xReturned == pdPASS);

  BaseType_t uart_tx_task_xReturned = xTaskCreate(uart_transmit_task,
                                                    "uartTxTask",
                                                    configMINIMAL_STACK_SIZE,
                                                    (void *) 1,
                                                    osPriorityBelowNormal,
                                                    &uart_tx_taskHandle);
  assert_param(uart_tx_task_xReturned == pdPASS);

  BaseType_t led_task_xReturned = xTaskCreate(led_control,
                                                    "LedCtrlTask",
                                                    configMINIMAL_STACK_SIZE,
                                                    (void *) 1,
                                                    osPriorityLow1,
                                                    &led_ctrl_taskHandle);

  assert_param(led_task_xReturned == pdPASS);

  BaseType_t init_task_xReturned = xTaskCreate(init_task,
                                                "InitTask",
                                                configMINIMAL_STACK_SIZE,
                                                (void *) 1,
                                                osPriorityHigh,
                                                &init_taskHandle);

  assert_param(init_task_xReturned == pdPASS);

  BaseType_t generate_signal_task_xReturned = xTaskCreate(generate_signal,
                                                 "GenerateSignalTask",
                                                 configMINIMAL_STACK_SIZE,
                                                 (void *) 1,
                                                 osPriorityAboveNormal,
                                                 &generate_signal_taskHandle);

  assert_param(generate_signal_task_xReturned == pdPASS);

  BaseType_t usb_task_xReturned = xTaskCreate(usb_task,
                                              "UsbTask",
                                              configMINIMAL_STACK_SIZE,
                                              (void *) 1,
                                              osPriorityHigh,
                                              &usb_taskHandle);

  assert_param(usb_task_xReturned == pdPASS);

  BaseType_t filter_task_xReturned = xTaskCreate(filter_task,
                                                "FilterTask",
                                                configMINIMAL_STACK_SIZE,
                                                (void *) 1,
                                                osPriorityAboveNormal1,
                                                &filter_taskHandle);

  assert_param(filter_task_xReturned == pdPASS);

  BaseType_t mic_task_xReturned = xTaskCreate(mic_task,
                                                   "MicTask",
                                                   configMINIMAL_STACK_SIZE,
                                                   (void *) 1,
                                                   osPriorityAboveNormal1,
                                                   &mic_taskHandle);

  assert_param(mic_task_xReturned == pdPASS);

  uart_tx_bufferHandle = xStreamBufferCreate(UART_TX_BUFFER_SIZE, 1);
  assert_param(uart_tx_bufferHandle != NULL);

  usb_bufferHandle = xStreamBufferCreate(USB_STREAM_SIZE, MIN_USB_STREAM_SIZE);
  assert_param(usb_bufferHandle != NULL);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */

  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
    UNUSED(argument);
    while (1) {
        osDelay(1);
    }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
* @brief Function implementing the inputCtrl thread.
* @param argument: Not used
* @retval None
*/
void init_task(void *argument)
{
    UNUSED(argument);
    uart_transmit(clear_code);
    uart_transmit(welcome_message);
    uart_transmit(main_menu_message);
    uart_receive();

    /* suspend all signal in tasks until the corresponding mode activated */
//    vTaskSuspend(generate_signal_taskHandle);

    calculate_lut();
    calculate_dc_points();
    calculate_amplitudes();
    init_filter_array();
    enable_mic_dma();

    // todo_fixed: change tasks with user
    vTaskSuspend(mic_taskHandle);

    vTaskDelete(NULL);
}


/**
* @brief Function implementing the inputCtrl thread.
* @param argument: Not used
* @retval None
*/
void input_control(void *argument)
{
    UNUSED(argument);
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        SEGGER_SYSVIEW_PrintfHost("rx_receive: %c", uart_rx_char);

        // process the char
        state_functions[program_state](uart_rx_char);

        // at the end re-enable dma port for another char
    }
}

/**
* @brief Function implementing the led_ctrl thread.
* @param argument: Not used
* @retval None
*/
void led_control(void *argument)
{
    UNUSED(argument);
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        SEGGER_SYSVIEW_PrintfHost("led control fired");

        led_channel_ctrl(RED_CHANNEL, (mode_states & (MIC_ENABLE | SIGNAL_ENABLE | ADC_ENABLE)));
        led_channel_ctrl(ORANGE_CHANNEL, (mode_states & (FILTER_ENABLE)));
        led_channel_ctrl(BLUE_CHANNEL, (mode_states & (DAC_ENABLE | DRIVER_ENABLE)));
        led_channel_ctrl(GREEN_CHANNEL, (mode_states & (USB_ENABLE)));
    }
}

/**
* @brief Function implementing the uartTxTask thread.
* @param argument: Not used
* @retval None
*/
void uart_transmit_task(void *argument)
{
    UNUSED(argument);
    size_t received_bytes;
    while (1) {
        received_bytes = xStreamBufferReceive(uart_tx_bufferHandle,
                                              uart_tx_buffer,
                                              UART_TX_BUFFER_SIZE,
                                              portMAX_DELAY);
        if (received_bytes > 0) {
            HAL_UART_Transmit_DMA(&huart2, &uart_tx_buffer[0], received_bytes);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    // DMA TX ISR will notify if transfer is completed
            memset(uart_tx_buffer, 0, UART_TX_BUFFER_SIZE);
        }
        else {
            // todo: is sleeping necessary?
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void generate_signal(void *argument) {
    UNUSED(argument);

    while (1) {
        if (mode_states & SIGNAL_ENABLE) {
            signal_functions[signal_settings.signal_type]();
        } else if (mode_states & MIC_ENABLE) {
            uint16_t *pdm_ptr = &mic_pdm_buffer[mic_pdm_buffer_state];
            PDM_Filter((uint8_t*) pdm_ptr, signal_buffer, &PDM1_filter_handler);
        }

        /* run filter task, if it is suspended pass it */
        if (mode_states & FILTER_ENABLE) {
            xTaskNotifyGive(filter_taskHandle);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        /* send signal buffer to usb_task */
        xStreamBufferSend(usb_bufferHandle,
                          signal_buffer,
                          SIGNAL_SIZE_BYTES,
                          portMAX_DELAY);
    }
}



void filter_task(void *argument) {
    UNUSED(argument);

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait until signal buffer is ready
        memcpy(filter_buffer.current_inputs, signal_buffer, SIGNAL_SIZE_BYTES);

        if (use_sos)    iir_filter_sos_direct_1(signal_buffer);
        else   filter_functions[current_filter.filter_type](signal_buffer);

        xTaskNotifyGive(generate_signal_taskHandle);
    }
}

void mic_task(void *argument) {
    UNUSED(argument);

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait until i2s callbacks fired
        uint16_t *pdm_ptr = &mic_pdm_buffer[mic_pdm_buffer_state];
        PDM_Filter((uint8_t*) pdm_ptr, signal_buffer, &PDM1_filter_handler);

//        interpolate();

        /* run filter task, if it is suspended -> pass it */
        if (mode_states & FILTER_ENABLE) {
            xTaskNotifyGive(filter_taskHandle);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        /* send signal buffer to usb_task */
        xStreamBufferSend(usb_bufferHandle,
                          signal_buffer,
                          SIGNAL_SIZE_BYTES,
                          portMAX_DELAY);
    }
}


void usb_task(void *argument) {
    UNUSED(argument);

    usb_init();
    while (1) {
        tud_task();
//        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* USER CODE END Application */

