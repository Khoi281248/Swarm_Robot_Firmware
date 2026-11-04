/**
 * @file    : protocol_slave.h
 * @brief   : provide api for protocol task
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#ifndef PROTOCOL_SLAVE_H
#define PROTOCOL_SLAVE_H

void protocol_task_start(UART_HandleTypeDef* huart, unsigned portBASE_TYPE prior);

#endif /* PROTOCOL_SLAVE_H */
