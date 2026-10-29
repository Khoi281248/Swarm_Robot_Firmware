/**
 * @file    : protocol.c
 * @brief   : commuinication protocol over UART
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#include "swarm_protocol.h"
#include "stdint.h"

#define	MAX_PROTOCOL_LEN	32

#define PROTOCOL_START_CODE	0xBF
/* Set of command code */
#define GO_TO_POINT_EXACT	0x01	// exact pose interpolation
#define GO_TO_POINT_FAST	0x02	// optimize for fastest trajectory
#define STABILIZE_AT_POINT	0x03	// stabilization at point

/* We are using fixed len package here to simplify the receive process,
* integrety check, and send process, all unused byte will not be forced clear or set.
* This create varied crc check.
* */
typedef struct {
	uint8_t head;
	uint8_t cmd;
	uint32_t body[MAX_PROTOCOL_LEN];
	uint16_t crc;
} protocol_package_t;
static protocol_package_t recv_package;
static protocol_package_t send_package;

typedef enum {
	WAIT_HEAD = 0,
	CHECK_CMD = 1,
	RECV_BODY = 2,
	CHECK_CRC = 3,
} protocol_state_machine;
static protocol_state_machine recv_state;
static uint32_t recv_body_count = 0;

/* Static function definition goes here */
static uint16_t compute_crc(const uint8_t *data, uint16_t len);

/* 
 * This function is designed to work over UART_REV interrupt method. 
 * As the state machine will be initialized as static variable, state machine will return
 * its state for caller wish to check ther operation status.
 * */
protocol_state_machine recieve_package(uint8_t recv_char)
{
	protocol_state_machine return_code =  recv_state;
	switch (recv_state) {
		case WAIT_HEAD:
			if (recv_char == PROTOCOL_START_CODE)
				recv_state = CHECK_CMD;
			break;
		case CHECK_CMD:
			if (recv_char & ~0x03 != 0) {
				recv_state = WAIT_HEAD;
			} else {
				recv_package.head = recv_char;
				recv_body_count &= 0x00;	// same as assign 0 anyway when compiled
				recv_state = RECV_BODY;
			}
			break;
		case RECV_BODY:
			if (recv_body_count++ < MAX_PROTOCOL_LEN) {
				recv_package.body[recv_body_count] = recv_char;
			} else {
				recv_state = CHECK_CRC;
			}
			break;
		case CHECK_CRC:
			uint16_t recv_crc = compute_crc(
					((uint8_t *)&recv_package)+1, 
					(uint16_t)(8+MAX_PROTOCOL_LEN));
			if (recv_package.crc == recv_crc)
				// Push to data ring, queue with RTOS
				;
			recv_state = WAIT_HEAD;
			break;
	}
	return return_code;
}

/*
 * Generate CRC 16 from software for consistent code generate and check,
 * Only verify package integrity for cmd and body, head code is not needed, 
 * it is part of package identifier itself.
 * Source: https://github.com/loiturong/OTA-update-for-IOT-device/blob/main/stm32/app/Core/Src/protocol.c
 * */
static uint16_t compute_crc(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= (uint16_t)(*data++ << 8);
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}
