/**
 * @file    : pid.c
 * @brief   : Implementation of Digitial PID for discrete system.
 * This PID controller is implemented with anti-windup and lower-pass filter,
 * set these two parameters to 0 to clear the effect of low-pass filter and anti-windup.
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#include "pid.h"

// PID internal state 
static float ek1 = 0; 
static float ek2 = 0;
static float uk  = 0;
static float uk1 = 0;
// PID control parameters
const static float det = 0.01;
const static float kp  = 1;
const static float ki  = 1;
const static float kd  = 1;
// Anti-windup and low-pass filter
const static float alpha = 1 / det;

pid_out_t pid_loop(float setpoint, float current)
{
	// Compute error at k
	float ek = setpoint - current;

	/* PID algorithm, adding anti-windup and low-pass filter by default */
	float up = kp*(ek - ek1);
	float ui = ki * det * (ek -ek1) / 2;
	ui = ui;	// TODO: Add anti-windup here
	float ud = kd * (ek - 2*ek1 + ek2) / det;
	ud = (1 - alpha)*ek + alpha * ud;
	uk = uk1 + up + ui + ud;

	// Update control value
	uk1 = uk;
	ek2 = ek1;
	ek1 = ek;
	
	return (uk >= 0) ? (pid_out_t){uk, 1} : (pid_out_t){-uk, 0};
}

