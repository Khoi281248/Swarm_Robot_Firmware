/**
 * @file    : software_crc.h
 * @brief   : software crc computation
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#ifndef SOFTWARE_CRC_H
#define SOFTWARE_CRC_H

#include <stdint.h>

uint16_t compute_crc(const uintptr_t *p_buffer, uint16_t len);

#endif /* SOFTWARE_CRC_H */
