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
#define init_code "\033[H\033[J"
#define clear_code "\033[2J"
#define clear_line "\33[2K\r"
#define red_code "\u001b[31m"
#define reset_color "\u001b[0m"


#define up_n(n) ("\033[" #n "A")
#define down_n(n) ("\033[" #n "B")
#define forward_n(n) ("\033[" #n "C")
#define backward_n(n) ("\033[" #n "D")
#define save_cursor "\033[s"
#define restore_cursor "\033[u"
#define erase_to_end "\033[K"


#define TWO_COMP(x) ((uint8_t) ((x & 0x80U) == 0x80U ? ~x + 0x01U : x))
#define TOGGLE_CHAR(x) (x = !x)

/* USER CODE END Private defines */

void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void printSerial(const char* msg);
void printNumber(uint32_t number);
void clearResetTerminal();


void printWelcomeMessage();
void ledControl();
void UserInputControl();
void printMenuMessage();
void busyWaitInput(uint8_t wait_for);
uint8_t generateSignal();
void generateHeaders(uint8_t *buf);
void transferSignal();


/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

