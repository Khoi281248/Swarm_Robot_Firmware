/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "FreeRTOS.h"
#include "queue.h"
#include "hardware_protocol.h"
#include "software_crc.h"

/* USER CODE END 0 */

UART_HandleTypeDef huart1;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
static protocol_package_t s_recv_package;
static protocol_recv_state_t s_recv_state;
static BaseType_t s_protocol_task_wake;
extern QueueHandle_t g_protocol_queue;

void protocol_receive_init(UART_HandleTypeDef* huart)
{
	// Init receive operation
	s_recv_state = WAIT_HEAD;
	HAL_UART_Receive_IT(huart, &s_recv_package.head, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	switch (s_recv_state) {
		case WAIT_HEAD:
			if (s_recv_package.head == PROTOCOL_START_CODE) {
				s_recv_state = CHECK_CMD;
				HAL_UART_Receive_IT(huart, &s_recv_package.cmd, 1);
			} else {
				HAL_UART_Receive_IT(huart, &s_recv_package.head, 1);
			}
			break;
		case CHECK_CMD:
			if ((s_recv_package.cmd & ~0x03) != 0) {
				s_recv_state = WAIT_HEAD;
				HAL_UART_Receive_IT(huart, &s_recv_package.head, 1);
			} else {
				s_recv_state = CHECK_CRC;
				/* Receive all remaining bytes
				 * id: 1 byte
				 * comd: 1 byte
				 * body: PROTOCOL_LEN * 4 (float 32 bit)
				 * crc: 2 byte
				 */
				HAL_UART_Receive_IT(huart, 
						&s_recv_package.id, 
						1 + 1 + (4 * PROTOCOL_LEN) + 2);
			}
			break;
		case CHECK_CRC:
			s_protocol_task_wake = pdFALSE;
			/* Wake the task to process this data */
			xQueueOverwriteFromISR(g_protocol_queue, &s_recv_package, &s_protocol_task_wake);
			if (s_protocol_task_wake == pdTRUE) 
				portYIELD_FROM_ISR((BaseType_t)NULL);
		default:
			s_recv_state = WAIT_HEAD;
			HAL_UART_Receive_IT(huart, &s_recv_package.head, 1);
			break;
	}
}

/* USER CODE END 1 */

