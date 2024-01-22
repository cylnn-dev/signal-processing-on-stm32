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
#include <math.h>
#include <string.h>
#include <stdio.h>

static const char welcome_message[] = "Welcome to Debug Interface!\n\n\r";
static const char menu_message[] = "Pick one of the following options\n\n\r1. See details about this program\n\r2. Basic LED control\n\r0. Reset\n\r";
static const char details_message[] = "!!!Under Construction!!!\n\rThis is a program about Digital signal processing\n\r";
static const char led_message[] = "LED Control System\n\r1. Orange\n\r2. Green\n\r3. Red\n\r4. Blue\n\r0. Back to Main Menu\n\r";
static const uint32_t powers_of_10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};


/* USER CODE END 0 */

UART_HandleTypeDef huart2;
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
    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_CIRCULAR;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_HIGH;
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
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

#define BYTE_PER_SAMPLE 4
#define SAMPLE_PER_PACKET 1024
#define TX_BUFFER_SIZE (BYTE_PER_SAMPLE * (SAMPLE_PER_PACKET + 2))  // two header for two half of the buffer
#define SAMPLING_RATE 10000  // in Hz
#define END_FILE 0xFFFF


#define N_FREQS 2
//#define PI_MUL2 M_PI * 2;


static uint8_t txBuffer[TX_BUFFER_SIZE] = {0};
float samplingPeriod = 1.0f / SAMPLING_RATE;
float signalAmp = 5;

//static float signalFreqs[N_FREQS] = {0};
static float signalFreqs[N_FREQS] = {523.25f, 622.25f};  // C and Eb524.25
static volatile uint8_t generateFirstHalf = 0x01U;
static float t = {0};

#define USE_FIR  1U

#if (USE_FIR == 1)
static const int degree = 67;  // it was 9

static const float coefs[] = {
        0.001474334626, 0.001444997615, 0.001647360274,  0.00135443639,0.0003735209466,
        -0.001364343218,-0.003734119702,-0.006379261613,-0.008729499765, -0.01008319762,
        -0.009747693315,-0.007214857731,-0.002337008016, 0.004544713534,  0.01255554706,
        0.02036528848,  0.02638283372,  0.02904852852,  0.02717159316,  0.02024319395,
        0.008653203957,-0.006255923305, -0.02232486196, -0.03689782694, -0.04728300124,
        -0.05126791447, -0.04757915437,  -0.0362056531, -0.01848832145, 0.003044291167,
        0.02507278882,  0.04406345636,  0.05689247698,  0.06142250076,  0.05689247698,
        0.04406345636,  0.02507278882, 0.003044291167, -0.01848832145,  -0.0362056531,
        -0.04757915437, -0.05126791447, -0.04728300124, -0.03689782694, -0.02232486196,
        -0.006255923305, 0.008653203957,  0.02024319395,  0.02717159316,  0.02904852852,
        0.02638283372,  0.02036528848,  0.01255554706, 0.004544713534,-0.002337008016,
        -0.007214857731,-0.009747693315, -0.01008319762,-0.008729499765,-0.006379261613,
        -0.003734119702,-0.001364343218,0.0003735209466,  0.00135443639, 0.001647360274,
        0.001444997615, 0.001474334626
};

static uint8_t filterBuffer[TX_BUFFER_SIZE] = {0};

void applyFIR(const int degree, const float *coefs) {
    float *fPtr = NULL;
    uint8_t *resultPtr = NULL;
    for (int i = (generateFirstHalf ? (0 + BYTE_PER_SAMPLE) : (TX_BUFFER_SIZE / 2) + BYTE_PER_SAMPLE);
    i < (generateFirstHalf ? ((TX_BUFFER_SIZE / 2) - BYTE_PER_SAMPLE) : TX_BUFFER_SIZE);
    i += BYTE_PER_SAMPLE) {
        fPtr = &txBuffer[i];
        float sum = 0;
        for (int j = 0; j < degree; ++j) {
            if ((i / 4) - j >= 0) {  // i/4 is the index of the array, if the array type is float
                sum += coefs[j] * *(fPtr - j);
            }
        }

        // lastly, write the result to the buffer, check also if buffer addr. is given to the DMA
        resultPtr = &sum;
        for (size_t j = 0; j < BYTE_PER_SAMPLE; ++j) {
            filterBuffer[i + j] = *(resultPtr + j);
        }
    }
}
#endif


void generateHeaders(uint8_t *buf) {
    for (int i = 0; i < BYTE_PER_SAMPLE; ++i) {
        buf[i] = 0xFF;
    }
    for (int i = (TX_BUFFER_SIZE / 2); i < (TX_BUFFER_SIZE / 2) + BYTE_PER_SAMPLE; ++i) {
        buf[i] = 0xFF;
    }
}

uint8_t generateSignal() {
    for (int i = (generateFirstHalf ? (0 + BYTE_PER_SAMPLE) : (TX_BUFFER_SIZE / 2) + BYTE_PER_SAMPLE);
    i < (generateFirstHalf ? ((TX_BUFFER_SIZE / 2) - BYTE_PER_SAMPLE) : TX_BUFFER_SIZE);
    i += BYTE_PER_SAMPLE) {
        float sum = 0;
        for (int j = 0; j < N_FREQS; ++j) {
            sum += cosf(2 * M_PI * signalFreqs[j] * t);
        }
        sum *= signalAmp;

        uint8_t *sumPtr = &sum;
        for (size_t j = 0; j < BYTE_PER_SAMPLE; ++j) {
            txBuffer[i + j] = *(sumPtr + j);
        }

        t += samplingPeriod;
    }

#if (USE_FIR == 1)
    applyFIR(degree, coefs);
#endif

    TOGGLE_CHAR(generateFirstHalf);
    return generateFirstHalf;
}

void transferSignal() {
    generateHeaders(txBuffer);

#if (USE_FIR == 1)
    generateHeaders(filterBuffer);
#endif

    generateSignal();
    generateSignal();
#if (USE_FIR == 1)
    HAL_UART_Transmit_DMA(&huart2, &filterBuffer[0], TX_BUFFER_SIZE);
#else
    HAL_UART_Transmit_DMA(&huart2, &txBuffer[0], TX_BUFFER_SIZE);
#endif
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
    while (generateFirstHalf != 0x01U) { HAL_UART_DMAPause(huart); }
    HAL_UART_DMAResume(huart);
    generateSignal();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    while (generateFirstHalf != 0x00U) { HAL_UART_DMAPause(huart); }
    HAL_UART_DMAResume(huart);
    generateSignal();
}

// legacy UART codes

void clearBuffer(uint8_t *buffer) {
    memset(buffer, 0, TX_BUFFER_SIZE);
}

void printSerial(const char* msg) {
    clearBuffer(txBuffer);
    for (int i = 0; i < strlen(msg); ++i) {
        txBuffer[i] = msg[i];  // maybe define more complex so give errors if something happens
    }
    HAL_UART_Transmit(&huart2, txBuffer, 1024, HAL_MAX_DELAY);
    clearBuffer(txBuffer);
}

void printNumber(uint32_t number) {
    clearBuffer(txBuffer);
    sprintf(txBuffer, "%lu", number);

    HAL_UART_Transmit(&huart2, txBuffer, sizeof(txBuffer), HAL_MAX_DELAY);
}

void clearResetTerminal() {
    HAL_UART_Transmit(&huart2, (uint8_t *) init_code, strlen(init_code), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *) clear_code, strlen(clear_code), HAL_MAX_DELAY);
}


void printWelcomeMessage() {
    clearResetTerminal();
    printSerial(welcome_message);
}
void printMenuMessage() {
    printSerial(menu_message);
}




/* USER CODE END 1 */
