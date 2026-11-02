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


typedef struct {
	float 	duty;	// float range from [0,1], need post-processing to get the TIM_PWM value
	uint8_t dir;	// direction to H-bridge
} pid_out_t;

pid_out_t pid_loop(float setpoint, float current);

#endif /* PID_H */
