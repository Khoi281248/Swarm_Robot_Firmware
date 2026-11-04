/**
 * @file    : protocol_slave.c
 * @brief   : imeplemtation of protocol task on slave side
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "hardware_protocol.h"
#include "software_crc.h"

static protocol_package_t s_send_package;
extern volatile float xc, yc, thetac, vc;
extern volatile float xr, yr, thetar, vr;
extern QueueHandle_t g_protocol_queue;
extern xSemaphoreHandle posture_mutex;
extern xSemaphoreHandle velocity_mutex;
extern xSemaphoreHandle ref_pos_mutex;

/* static helpers */
static inline void protocol__send_in_time(void);
static inline TickType_t protocol__update_in_time(void);

void protocol_task_start(UART_HandleTypeDef* huart, unsigned portBASE_TYPE prior)
{
	s_send_package.head = PROTOCOL_START_CODE;
	g_protocol_queue = xQueueCreate(1, sizeof(protocol_package_t));
	protocol_init(huart);
	
	xTaskCreate((TaskFunction_t)protocol_task, "protocol_data_update", 512, (void *) NULL, 
			(UBaseType_t) prior, (TaskHandle_t *) NULL);
}

void protocol_task(void)
{
	for (;;) {
		/* blocking wait for uart interrupt push data to send package */
		xQueueReceive(g_protocol_queue, &s_send_package, portMAX_DELAY);
		uint16_t recv_crc = compute_crc((uintptr_t *)&s_recv_package, 
				sizeof(protocol_package_t)-sizeof(uint16_t));
		/* Other system status added latter... maybe */
		if (recv_crc != s_send_package.crc)
			s_send_package.status = BAD_CRC;
		else
			s_send_package.status = GOOD_RES;
		if (!protocol__send_in_time()) {
			s_send_package.status = BROKEN_RES;
		}
		s_send_package.crc = compute_crc((uintptr_t *)&s_recv_package, 
				sizeof(protocol_package_t)-sizeof(uint16_t));
		HAL_UART_Transmit_IT(huart, (uint8_t *)&s_send_package, sizeof(protocol_package_t));
	}
}

static inline TickType_t protocol__update_in_time(void)
{
	TickType_t start = xTaskGetTickCount();
	/* try update reference posture and velocity */
	if (xSemaphoreTake(ref_pos_mutex, pdMS_TO_TICKS(PROTOCOL_TIME_OUT)) == pdTRUE) {
		xr 	= s_send_package.body[0];
		yr 	= s_send_package.body[1];
		thetar 	= s_send_package.body[2];
		vr 	= s_send_package.body[3];
		if (xSemaphoreGive(ref_pos_mutex) != pdTRUE) {	
			/* This path is unreachable as 
			 * we must have had the lock to able to call Give api
			 * There must be something wrong in the system to reach here
			 * so this part is for debug
			 */
#ifdef DEBUG
			while (1);
#endif
		}
		return pdMS_TO_TICKS(PROTOCOL_TIME_OUT) - (xTaskGetTickCount() - start);
	} else {
		return 0;
	}
}

static inline uint8_t protocol__send_in_time(void)
{
	TickType_t start, remain;
	start = xTaskGetTickCount();
	remain = protocol__update_in_time();
	if (remain == 0) return 0;
	/* try update posture */
	if (xSemaphoreTake(posture_mutex, remain) == pdTRUE) {
		s_send_package.body[0] = xc;
		s_send_package.body[1] = yc;
		s_send_package.body[2] = thetac;
		xSemaphoreTake(posture_mutex, remain)
		if (xSemaphoreGive(posture_mutex) != pdTRUE) {
#ifdef DEBUG
			while (1);
#endif
		}

	} else {
		return 0;
	}
	/* try update velocity */
	remain -= (xTaskGetTickCount() - start);
	if (xSemaphoreTake(velocity_mutex, remain) == pdTRUE) {
		s_send_package.body[3] = vc;
		if (xSemaphoreGive(velocity_mutex) != pdTRUE) {
#ifdef DEBUG
			while (1);
#endif
		}

	} else {
		return 0;
	}
	return 1;
}
