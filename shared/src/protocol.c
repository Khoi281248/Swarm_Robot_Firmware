/**
 * @file    : protocol.c
 * @brief   : commuinication protocol over UART
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#include "swarm_protocol.h"

#define MAX_RETRIES		3
#define	MAX_PROTOCOL_LEN	32

#define PROTOCOL_START_CODE	0xAB

/* ACK-NACK code */
#define PROTOCOL_ACK_CODE	0xBC
#define PROTOCOL_NACK_CODE	0xCD

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

/* Static variables declaration goes here */
static protocol_package_t s_recv_package;
static protocol_package_t s_send_package = {PROTOCOL_START_CODE};	// reduce operations
static protocol_recv_state_t s_recv_state;
static uint32_t s_recv_body_count = 0;
static char *g_p_recv_byte;

/* Static function declaration goes here */
static uint16_t compute_crc(const protocol_package_t *p_package);

/* 
 * This function is designed to work over UART_REV interrupt method. 
 * As the state machine will be initialized as static variable, state machine will return
 * its state for caller wish to check ther operation status.
 * */
protocol_recv_state_t protocol_recieve(void)
{
	protocol_recv_state_t return_code =  recv_state;
	switch (s_recv_state) {
		case WAIT_HEAD:
			if (g_p_recv_byte == PROTOCOL_START_CODE)
				s_recv_state = CHECK_CMD;
			break;
		case CHECK_CMD:
			if (g_p_recv_byte & ~0x03 != 0) {
				s_recv_state = WAIT_HEAD;
			} else {
				s_recv_package.head = g_p_recv_byte;
				s_recv_body_count &= 0x00;	// same as assign 0 anyway when compiled
				s_recv_state = RECV_BODY;
			}
			break;
		case RECV_BODY:
			if (s_recv_body_count++ < MAX_PROTOCOL_LEN) {
				s_recv_package.body[s_recv_body_count] = p_recv_by;
			} else {
				s_recv_state = CHECK_CRC;
			}
			break;
		case CHECK_CRC:
			uint16_t recv_crc = compute_crc(&s_recv_package);
			if (s_recv_package.crc == g_p_recv_byte) {
				// Push to data ring, queue with RTOS
				transmit_data(&PROTOCOL_ACK_CODE);
			} else {
				transmit_data(&PROTOCOL_NACK_CODE);
			}
			s_recv_state = WAIT_HEAD;
			break;
		case RECV_ACK:
		case default:
			break;
	}
	return return_code;
}

protocol_send_state_t protocol_send(uint8_t cmd_code, float x, float y, float theta, float v)
{
	protocol_send_state_t status = BUIDLING;
	s_send_package.cmd = cmd_code;
	s_send_package.body[0] = x;
	s_send_package.body[1] = y;
	s_send_package.body[2] = theta;
	s_send_package.body[3] = v;
	s_send_package.crc = compute_crc(&s_send_package);

	//for (int i = 0; i < MAX_RETRIES; i++) {
	//	; // Send package with UART, this operation does not require interrupt and is supported with HAL
	//}
	//
	return status;
}

/*
 * Generate CRC 16 from software for consistent code generate and check,
 * Only verify package integrity for cmd and body, head code is not needed, 
 * it is part of package identifier itself.
 * Source: https://github.com/loiturong/OTA-update-for-IOT-device/blob/main/stm32/app/Core/Src/protocol.c
 * modifies:
 * - change input to the package structure
 * - change package receive handler
 * */
static uint16_t compute_crc(const protocol_package_t *p_package)
{
	/* Adding package structure specific handler */
	uint8_t *p_data = ((uint8_t*)p_package)+1;		// drop head
	uint16_t len = MAX_PROTOCOL_LEN + 8;			// body+cmd

	uint16_t crc = 0xFFFF;
	while (len--) {
		crc ^= (uint16_t)(*p_data++ << 8);
		for (uint8_t i = 0; i < 8; i++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
		}
	}
	return crc;
}
