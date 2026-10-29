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

#endif /* SWARM_PROTOCOL_H */
