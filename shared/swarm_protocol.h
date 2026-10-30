/**
 * swarm_protocol.h
 *
 * Giao thuc UART giua STM32F103 va ESP32-C3 (USART1 <-> UART1, 115200 8N1).
 * File nay dung chung cho ca hai firmware:
 *   - stm32/CMakeLists.txt     : target_include_directories(... ../shared)
 *   - esp32/main/CMakeLists.txt: INCLUDE_DIRS "../../shared"
 *
 * Chi dung kieu co kich thuoc co dinh (<stdint.h>) va struct packed,
 * vi STM32 (ARM) va ESP32-C3 (RISC-V) bien dich bang hai compiler khac nhau.
 *
 * TODO: dinh nghia khung goi tin (byte dau, loai goi, do dai, du lieu, CRC).
 */
#ifndef SWARM_PROTOCOL_H
#define SWARM_PROTOCOL_H

#include <stdint.h>

#define SWARM_UART_BAUD     115200U

/* Set of command code */
#define GO_TO_POINT_EXACT	0x01	// exact pose interpolation
#define GO_TO_POINT_FAST	0x02	// optimize for fastest trajectory
#define STABILIZE_AT_POINT	0x03	// stabilization at point
#define UPDATE_CURRENT_POSE	0x04	// send current computed posture

/* Global byte pointer, received byte from UART will be pushed to this pointer */
extern char *p_recv_byte;

/* define protocol structs, enums */
typedef enum {
	REVC_ACK  = -1,
	WAIT_HEAD = 0,
	CHECK_CMD = 1,
	RECV_BODY = 2,
	CHECK_CRC = 3,
} protocol_recv_state_t;

typedef enum {
	SEND_DROP = 0,
	SEND_DONE = 1,
	BUILDING = 2,
	WAIT_ACK = 3,
} protocol_send_state_t;

typedef enum {
	FAILED 	= 0,
	SUCCESS = 1,
} trans_recv_status_t;

/* API provided to application */
protocol_recv_state_t protocol_recieve(void);
protocol_send_state_t protocol_send(uint8_t cmd_code, 
		float x, float y, float theta, float v);

/* Wrapper for UART transfer that require application to defines its own logic */
trans_recv_status_t transmit_data(char *byte, uint32_t len);
/* This is a wrapper for UART receive Interrupt mode */
trans_recv_status_t receive_data_it(char *byte);

#endif /* SWARM_PROTOCOL_H */
