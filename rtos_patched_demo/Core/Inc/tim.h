/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
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
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdbool.h>

/* USER CODE END Includes */

extern TIM_HandleTypeDef htim3;

extern TIM_HandleTypeDef htim4;

/* USER CODE BEGIN Private defines */
#define GREEN_CHANNEL   TIM_CHANNEL_1
#define ORANGE_CHANNEL  TIM_CHANNEL_2
#define RED_CHANNEL     TIM_CHANNEL_3
#define BLUE_CHANNEL    TIM_CHANNEL_4


/* USER CODE END Private defines */

void MX_TIM3_Init(void);
void MX_TIM4_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN Prototypes */
HAL_StatusTypeDef led_channel_ctrl(uint16_t channel, bool state);
void start_tim3_dma(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

