/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <string.h>
#include "tim.h"



/* USER CODE END 0 */

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Stream5;
    hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
#define TX_SIZE 1024
#define RX_SIZE 1024
uint8_t uart_tx_buff[TX_SIZE + 1] = {};
uint8_t uart_rx_buff[1] = {0};
static volatile dma_states uart_dma_tx_state = DMA_READY;
static volatile dma_states uart_dma_rx_state = DMA_READY;

struct ModeList {
    uint8_t program_reset;
    uint8_t mic_in;
    uint8_t signal_in;
    uint8_t usb_out;
    uint8_t dac_out;
    uint8_t filter_apply;
    uint8_t debug_mode;
};

struct ModeList mode_list = {0,0,0,0,0,0, 1};

static void mode_select_loop();
static void change_led_states();
static void change_feed();

void clear_buffer(void *buffer, size_t size) {
    memset(buffer, 0, size);
}

void clear_reset_terminal() {
    print_serial(init_code);
    print_serial(clear_code);
}

void print_serial(const char* msg) {
    while (uart_dma_tx_state != DMA_READY) { __NOP(); }
    uart_dma_tx_state = DMA_BUSY;

    clear_buffer(uart_tx_buff, TX_SIZE);
    strcpy((char *) uart_tx_buff, msg);
    HAL_UART_Transmit_DMA(&huart2, uart_tx_buff, strlen(msg));
}

void print_debug(const char* msg) {
    if (mode_list.debug_mode) {
        print_serial(msg);
    }
}

void print_welcome_message() {
    clear_reset_terminal();
    print_serial(welcome_message);
}

void print_menu_message() {
    print_serial(menu_message);
    request_input();
}


void request_input() {
    while (uart_dma_tx_state != DMA_READY) { __NOP(); }
    clear_buffer(uart_rx_buff, RX_SIZE);
    print_serial("\n\r> ");
    HAL_UART_Receive_DMA(&huart2, uart_rx_buff, 1);
}

void user_input_control() {
    if (uart_dma_rx_state == RX_RECEIVED) {
        uint8_t user_input = uart_rx_buff[0];
        uart_dma_rx_state = DMA_READY;

        clear_reset_terminal();

        switch (user_input) {
            case '1':
                print_serial(mode_message);
                mode_select_loop();
                request_input();
                break;
            case '2':
                mode_list.debug_mode ^= 1;
                mode_list.debug_mode ? (print_serial("\n\rDEBUG mode enabled\n\r")) : (print_serial(
                        "\n\rDEBUG mode disabled\n\r"));
                print_serial(return_main_menu_message);
                request_input();
                break;
            case '9':
                print_serial(details_message);
                request_input();
                break;
            case '0':
                print_serial(menu_message);
                request_input();
                break;
            default:
                print_serial(invalid_input_error_message);
                print_serial(menu_message);
                request_input();
                break;
        }

        change_led_states();
        change_feed();

    }
}

void change_feed() {
    if (mode_list.mic_in) {
        feed = &feed_mic_data;
    } else if (mode_list.signal_in) {
        feed = &feed_signal_data;
    } else {
        feed = &feed_nothing;
    }
}

void change_led_states() {
    /* control LEDs after possible mod selection */
    /*
    * TIM4 CHANNELS FOR LED'S
    * 0 -> green
    * 1 -> orange
    * 2 -> red
    * 3 -> blue
    */
    tim_channel_control(3, mode_list.signal_in);
    tim_channel_control(2, mode_list.mic_in);
    tim_channel_control(1, mode_list.filter_apply);
    tim_channel_control(0, (mode_list.usb_out || mode_list.dac_out));
}

void mode_select_loop() {
    uint8_t selection_finished = 0;
    while (!selection_finished) {
        request_input();
        while (uart_dma_rx_state != RX_RECEIVED) { __NOP(); } // todo: this busy wait is not a good idea
        uart_dma_rx_state = DMA_READY;
        uint8_t mode_select_input = uart_rx_buff[0];
        switch (mode_select_input) {
            case '1':
                mode_list.mic_in ^= 1;  // using ^= (xor) toggles the previous value, very easy to add
                mode_list.signal_in = 0;
                break;
            case '2':
                mode_list.signal_in ^= 1;
                mode_list.mic_in = 0;
                break;
            case '3':
                mode_list.usb_out ^= 1;
                break;
            case '4':
                mode_list.dac_out ^= 1;
                break;
            case '5':
                mode_list.filter_apply ^= 1;
                break;
            case '0':
                selection_finished = 1;
                break;
            default:
                print_serial("\n\r mode not understood. Try again\n\r");
                break;
        }
        /* show the user which mod is selected by simply printing the rx_value */
        char mode_select_char = mode_select_input + '\0';
        print_serial(&mode_select_char);
    }
    clear_reset_terminal();
    print_serial(menu_message);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        uart_dma_rx_state = RX_RECEIVED;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        uart_dma_tx_state = DMA_READY;
    }
}

/* USER CODE END 1 */
