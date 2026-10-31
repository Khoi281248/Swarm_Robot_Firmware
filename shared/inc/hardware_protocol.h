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

#define SWARM_UART_BAUD		115200U


#define MAX_RETRIES		3
#define	PROTOCOL_LEN	8

#define PROTOCOL_START_CODE	0xAB

/* Status code */
#define PROTOCOL_ACK_CODE	0xBC
#define PROTOCOL_NACK_CODE	0xCD

/* Set of command code */
#define GO_TO_POINT_EXACT	0x01	// exact pose interpolation
#define GO_TO_POINT_FAST	0x02	// optimize for fastest trajectory
#define STABILIZE_AT_POINT	0x03	// stabilization at point

/* Global byte pointer, received byte from UART will be pushed to this pointer */
extern char *p_recv_byte;

/* define protocol structs, enums */

typedef enum {
	WAIT_HEAD = 0,
	CHECK_CMD = 1,
	CHECK_CRC = 2
} protocol_recv_state_t;

typedef enum {
	CMD_SEND,
	GOOD_RES,
	BAD_CRC,
} protocol_status_t;

/* We are using fixed len package here to simplify the receive process,
* integrety check, and send process, all unused byte will not be forced clear or set.
* This create varied crc check.
* */
typedef struct {
	uint8_t head;
	uint8_t cmd;
	uint8_t id;
	uint8_t status;
	uint32_t body[PROTOCOL_LEN];
	uint16_t crc;
} protocol_package_t;

#endif /* SWARM_PROTOCOL_H */
