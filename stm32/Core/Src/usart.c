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
static protocol_package_t s_send_package;
static protocol_recv_state_t s_recv_state;
extern float x, y, theta, v;

void init_protocol(UART_HandleTypeDef* huart)
{
	// Init receive operation
	s_recv_state = WAIT_HEAD;
	HAL_UART_Receive_IT(huart, &s_recv_package.head, 1);
}

void protocol_send(UART_HandleTypeDef* huart, protocol_status_t status)
{
	s_send_package.cmd = s_recv_package.cmd;
	s_send_package.id = s_recv_package.id;
	/* These part belongs to a gloable shared variables
	 * But read operation does not require to lock the variables
	 * Consider doing so if the data hands out unstable
	 */
	if (status == BAD_CRC) {
		s_send_package.body[0] = 0;
		s_send_package.body[1] = 0;
		s_send_package.body[2] = 0;
		s_send_package.body[3] = 0;
	} else {
		s_send_package.body[0] = x;
		s_send_package.body[1] = y;
		s_send_package.body[2] = theta;
		s_send_package.body[3] = v;
	}
	s_send_package.crc = compute_crc((uintptr_t *)&s_recv_package, 
					sizeof(protocol_package_t)-sizeof(uint16_t));
	HAL_UART_Transmit_IT(huart, (uint8_t *)&s_send_package, sizeof(protocol_package_t));
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
				HAL_UART_Receive_IT(huart, &s_recv_package.id, 1);
				HAL_UART_Receive_IT(huart, &s_recv_package.status, 1);
				HAL_UART_Receive_IT(huart, (uint8_t *)&s_recv_package.body, 4*PROTOCOL_LEN);
				HAL_UART_Receive_IT(huart, (uint8_t *)&s_recv_package.crc, 2);
			}
			break;
		case CHECK_CRC:
			uint16_t recv_crc = compute_crc(
					(uintptr_t *)&s_recv_package, 
					sizeof(protocol_package_t)-sizeof(uint16_t));
			if (recv_crc == s_recv_package.crc) {
				protocol_send(huart, GOOD_RES);
			} else {
				protocol_send(huart, BAD_CRC);
			}
			s_recv_state = WAIT_HEAD;
			break;
		default:
			s_recv_state = WAIT_HEAD;
			HAL_UART_Receive_IT(huart, &s_recv_package.head, 1);
			break;
	}
}

/* USER CODE END 1 */

