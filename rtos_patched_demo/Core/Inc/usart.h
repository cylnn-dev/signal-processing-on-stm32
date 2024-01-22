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

//#define init_code "\033[H\033[J"
//#define clear_code "\033[2J"
#define clear_code "\033[H\033[J\033[2J"

#define UART_TX_BUFFER_SIZE 512


/* USER CODE END Private defines */

void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void uart_transmit(const char* message);
void uart_receive(void);




static const char welcome_message[] = "\n\n\rWelcome to the Digital Signal Processing Project!\n\n\r"
                               "This project is intended to use only for educational purposes. "
                               "It demonstrates signal processing and simple command interface capabilities "
                               "of STM32 based MCUs. Please note, this project operates under the MIT license, "
                               "promoting sharing and learning. Happy Coding!\n\r\0";

static const char about_message[] = "This project is intended to use only for educational purposes. "
                                   "It demonstrates signal processing and simple command interface capabilities "
                                   "of STM32 based MCUs. Please note, this project operates under the MIT license, "
                                   "promoting sharing and learning. Happy Coding!\n\n\r"
                                   "0. save and return to main menu\n\n\r\0";

static const char main_menu_message[] = "\n\n\r--- Main Menu ---\n\n\r"
                                        "1. signal processing settings\n\r"
                                        "2. signal generator settings\n\r"
                                        "3. filter settings\n\r"
                                        "9. About Program\n\n\r"
                                        "0. show main menu\n\n\r\0";

static char mode_menu_message[] = "--- Available Modes ---\n\n\r"
                                 "-- IN Source --\n\r"
                                 "1. microphone  -\n\r"
                                 "2. generated signal  -\n\r"
                                 "3. ADC  -\n\r"
                                 "-- OUT Sources --\n\r"
                                 "4. USB  -\n\r"
                                 "5. DAC  -\n\r"
                                 "6. audio driver  -\n\r"
                                 "-- Processing --\n\r"
                                 "7. apply filter  -\n\n\r"
                                 "0. save and return to main menu\n\n\r\0";

static char signal_menu_message[] = "--- Generated Signal Settings ---\n\n\r"
                                  "1. signal_type: COS     \n\r"
                                  "2. frequencies:                                                    \n\r"
                                  "3. amplitude:      \n\r"
                                  "4. sample_period: 1/48000\n\r"
                                  "5. dc_point:       \n\r"
                                  "6. duty_cycle: 0\n\n\r"
                                  "0. save and return to main menu\n\n\r\0";


static char filter_menu_message[] = "--- Filter Settings ---\n\n\r"
                                    "1. direct_form/sos: DIR\n\r"
                                    "2. cycle through\n\r"
                                    "--- current filter ---\n\n\r"
                                    "name: 00000000000000000000000000000000\n\n\r"
                                    "0. save and return to main menu\n\n\r\0";

//{54, 77, 96, 118, 136, 156}

#define MENU_POSITIONS_SIZE 7
static const uint32_t mode_menu_positions[MENU_POSITIONS_SIZE] = {58, 82, 93, 123, 134, 154, 192};

#define SIGNAL_POSITIONS_SIZE 6
static const uint32_t signal_menu_positions[SIGNAL_POSITIONS_SIZE] = {52, 78, 145, 155, 192, 156 + 4};

#define FILTER_POSITIONS_SIZE 2
static const uint32_t filter_menu_positions[FILTER_POSITIONS_SIZE] = {46, 100};



static const char not_understood_message[] = "\n\rthe input seems incorrect! Try again\n\n\r\0";
static const char wait_input_message[] = "> \0";
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

