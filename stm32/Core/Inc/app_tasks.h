/**
 * @file    : app_tasks.h
 * @brief   : Declare application task config function
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#ifndef APP_TASKS_H
#define APP_TASKS_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void pid_loop_task_start(unsigned portBASE_TYPE prior);
void tracking_task_start(unsigned portBASE_TYPE prior);

#endif /* APP_TASKS_H */
