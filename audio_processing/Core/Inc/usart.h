/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
#define init_code "\033[H\033[J"
#define clear_code "\033[2J"


void print_serial(const char* msg);
void clear_reset_terminal();
static const char menu_message[] = "Pick one of the following options\n\n\r"
                                   "1. Mode select\n\r"
                                   "2. Enable/Disable Debug Mode\n\r"
                                   "9. Details about the program\n\r"
                                   "0. Show the Menu\n\r";
static const char mode_message[] = "\n\r--- Available Modes ---\n\n\r"
                                   "-- IN Sources --\n\r"
                                   "1. microphone\n\r"
                                   "2. signal\n\n\r"
                                   "-- OUT Sources --\n\r"
                                   "3. USB\n\r"
                                   "4. DAC\n\n\r"
                                   "-- Processing --\n\r"
                                   "5. apply filter\n\n\r"
                                   "0. save and return to main menu";

static const char welcome_message[] = "Welcome to Audio Processing Interface!\n\n\r";
static const char details_message[] = "\n\rThis is a program about digital signal processing. (Work in Progress!)\n\n\r0. return to the main menu\n\n\r";
static const char invalid_input_error_message[] = "\n\rInvalid input received. Try again!\n\n\r";
static const char return_main_menu_message[] = "\n\r0. return to main menu\n\r";

static const char led_message[] = "LED Control System\n\r1. Orange\n\r2. Green\n\r3. Red\n\r4. Blue\n\r0. Back to Main Menu\n\r";

void request_input();
void clear_buffer(void *buffer, size_t size);
void print_debug(const char* msg);
void print_welcome_message();
void led_control();
void user_input_control();
void print_menu_message();
void busy_wait_for_input(uint8_t wait_for);
extern void feed_mic_data(void);
extern void feed_signal_data(void);
extern void (*feed)(void);
extern void feed_nothing(void);

typedef enum {
    DMA_READY = 0x00U,
    DMA_BUSY = 0x01U,
    RX_RECEIVED = 0X02U,
} dma_states;

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

