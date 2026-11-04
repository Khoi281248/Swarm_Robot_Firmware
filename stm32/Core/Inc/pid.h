/**
 * @file    : pid.h
 * @brief   : Digitial PID public api
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#ifndef PID_H
#define PID_H

#include <stdint.h>
#include "FreeRTOS.h"

void pid_loop_task_start(unsigned portBASE_TYPE prior);
void pid_update_setpoint(void);

#endif /* PID_H */
