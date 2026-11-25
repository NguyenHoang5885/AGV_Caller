/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
#define CONFIG_ADDR  0x0801F800 // Khai báo địa chỉ flask

//========IO TASK===========//
typedef struct {
	char *name;
    GPIO_TypeDef *port;
    uint16_t pin;
} GPIO;


typedef struct{
	uint8_t Input_Data;
	uint8_t Output_Data;
	uint8_t Button_Data;
	uint8_t Led_Data;
}IO_State_t;

#define TX2_SIZE 20
#define RX2_SIZE 256

#define TX3_SIZE 256
#define RX3_SIZE 8

#define TX5_SIZE 15
#define RX5_SIZE 15

#define ILLEGAL_FUNCTION  0x01

//SIGNAL HARDWARE INTERRUPT
#define SIG_BT1 (1U << 0)
#define SIG_BT2 (1U << 1)
#define SIG_BT3 (1U << 2)
#define SIG_BT4 (1U << 3)
#define SIG_SCAN_INPUT (1U << 4)

#define SIG_RST_WIFI (1U << 5)
#define SIG_ACCESS_AT (1U << 6)

#define SIG_DMA_UART2 (1U << 7 )
#define SIG_DMA_UART3 (1U << 8)
#define SIG_IT_UART5 (1U << 9)

//SIGNAL SOFTWARE INTERRUPT
#define SIG_IOQUEUE_TO_TCP (1U << 10)
#define SIG_IOQUEUE_TO_HTTP (1U << 11)
#define SIG_IOQUEUE_TO_AT (1U << 12)
#define SIG_IOQUEUE_TO_RTU (1U << 13)

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BT1_Pin GPIO_PIN_0
#define BT1_GPIO_Port GPIOC
#define BT1_EXTI_IRQn EXTI0_IRQn
#define BT2_Pin GPIO_PIN_1
#define BT2_GPIO_Port GPIOC
#define BT2_EXTI_IRQn EXTI1_IRQn
#define BT3_Pin GPIO_PIN_2
#define BT3_GPIO_Port GPIOC
#define BT3_EXTI_IRQn EXTI2_IRQn
#define BT4_Pin GPIO_PIN_3
#define BT4_GPIO_Port GPIOC
#define BT4_EXTI_IRQn EXTI3_IRQn
#define RST_WIFI_Pin GPIO_PIN_1
#define RST_WIFI_GPIO_Port GPIOA
#define USART2_TX_APP_Pin GPIO_PIN_2
#define USART2_TX_APP_GPIO_Port GPIOA
#define USART2_RX_APP_Pin GPIO_PIN_3
#define USART2_RX_APP_GPIO_Port GPIOA
#define IN1_Pin GPIO_PIN_4
#define IN1_GPIO_Port GPIOA
#define IN2_Pin GPIO_PIN_5
#define IN2_GPIO_Port GPIOA
#define IN3_Pin GPIO_PIN_6
#define IN3_GPIO_Port GPIOA
#define IN4_Pin GPIO_PIN_7
#define IN4_GPIO_Port GPIOA
#define IN5_Pin GPIO_PIN_4
#define IN5_GPIO_Port GPIOC
#define IN6_Pin GPIO_PIN_5
#define IN6_GPIO_Port GPIOC
#define IN7_Pin GPIO_PIN_0
#define IN7_GPIO_Port GPIOB
#define IN8_Pin GPIO_PIN_1
#define IN8_GPIO_Port GPIOB
#define USART3_TX_SERVER_Pin GPIO_PIN_10
#define USART3_TX_SERVER_GPIO_Port GPIOB
#define USART3_RX_SERVER_Pin GPIO_PIN_11
#define USART3_RX_SERVER_GPIO_Port GPIOB
#define OUT1_Pin GPIO_PIN_12
#define OUT1_GPIO_Port GPIOB
#define OUT2_Pin GPIO_PIN_13
#define OUT2_GPIO_Port GPIOB
#define OUT3_Pin GPIO_PIN_14
#define OUT3_GPIO_Port GPIOB
#define OUT4_Pin GPIO_PIN_15
#define OUT4_GPIO_Port GPIOB
#define OUT5_Pin GPIO_PIN_6
#define OUT5_GPIO_Port GPIOC
#define OUT6_Pin GPIO_PIN_7
#define OUT6_GPIO_Port GPIOC
#define OUT7_Pin GPIO_PIN_8
#define OUT7_GPIO_Port GPIOC
#define OUT8_Pin GPIO_PIN_9
#define OUT8_GPIO_Port GPIOC
#define BT_RST_WIFI_Pin GPIO_PIN_8
#define BT_RST_WIFI_GPIO_Port GPIOA
#define BT_RST_WIFI_EXTI_IRQn EXTI9_5_IRQn
#define UART5_TX_485_Pin GPIO_PIN_12
#define UART5_TX_485_GPIO_Port GPIOC
#define UART5_RX_485_Pin GPIO_PIN_2
#define UART5_RX_485_GPIO_Port GPIOD
#define LED_BT1_Pin GPIO_PIN_6
#define LED_BT1_GPIO_Port GPIOB
#define LED_BT2_Pin GPIO_PIN_7
#define LED_BT2_GPIO_Port GPIOB
#define LED_BT3_Pin GPIO_PIN_8
#define LED_BT3_GPIO_Port GPIOB
#define LED_BT4_Pin GPIO_PIN_9
#define LED_BT4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
