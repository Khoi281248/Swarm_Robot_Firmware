/**
 * @file    : protocol.c
 * @brief   : Generate CRC 16 from software for consistent code generate and check,
 * Only verify package integrity for cmd and body, head code is not needed, 
 * it is part of package identifier itself.
 * Source: https://github.com/loiturong/OTA-update-for-IOT-device/blob/main/stm32/app/Core/Src/protocol.c
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#include "software_crc.h"

uint16_t compute_crc(const uintptr_t *p_buffer, uint16_t len)
{
	uint16_t crc = 0xFFFF;
	while (len--) {
		crc ^= (uint16_t)(*p_buffer++ << 8);
		for (uint8_t i = 0; i < 8; i++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
		}
	}
	return crc;
}
