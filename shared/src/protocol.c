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
static protocol_package_t recv_package;
static protocol_package_t send_package = {PROTOCOL_START_CODE};	// reduce operations
static protocol_recv_state_t recv_state;
static uint32_t recv_body_count = 0;
static protocol_send_state_t send_state;
static char *p_recv_byte;

/* Static function declaration goes here */
static uint16_t compute_crc(const protocol_package_t *package);

/* 
 * This function is designed to work over UART_REV interrupt method. 
 * As the state machine will be initialized as static variable, state machine will return
 * its state for caller wish to check ther operation status.
 * */
protocol_recv_state_t recieve_package(uint8_t *recv_char)
{
	protocol_recv_state_t return_code =  recv_state;
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
			uint16_t recv_crc = compute_crc(&recv_package);
			if (recv_package.crc == recv_crc) {
				// Push to data ring, queue with RTOS
				// Sending ACK code
				recv_state = WAIT_HEAD;
			}
			break;
		case BLOCKING:
		case default:
			break;
	}
	return return_code;
}

protocol_send_state_t send_command(uint8_t cmd_code, float x, float y, float theta, float v)
{
	protocol_send_state_t status = BUIDLING;
	send_package.cmd = cmd_code;
	send_package.body[0] = x;
	send_package.body[1] = y;
	send_package.body[2] = theta;
	send_package.body[3] = v;
	send_package.crc = compute_crc(&send_package);


	for (int i = 0; i < MAX_RETRIES; i++) {
		; // Send package with UART, this operation does not require interrupt and is supported with HAL
	}
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
static uint16_t compute_crc(const protocol_package_t *package)
{
	/* Adding package structure specific handler */
	uint8_t *data_p = ((uint8_t*)package)+1;		// drop head
	uint16_t len = MAX_PROTOCOL_LEN + 8;			// body+cmd

	uint16_t crc = 0xFFFF;
	while (len--) {
		crc ^= (uint16_t)(*data_p++ << 8);
		for (uint8_t i = 0; i < 8; i++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
		}
	}
	return crc;
}
