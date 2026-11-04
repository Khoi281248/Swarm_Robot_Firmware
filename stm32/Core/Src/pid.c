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
#include "task.h"
#include "semphr.h"

/* store internal digital PID state */
typedef struct {
	float uk1;
	float ek1;
	float ek2;
	float setpoint;
	const float kp;
	const float ti;
	const float td;
	const float alpha;
} pid_state_t;

typedef struct {
	float 	duty;	// float range from [0,1], need post-processing to get the TIM_PWM value
	uint8_t dir;	// direction to H-bridge
} pid_out_t;

// Control parameters
const static float kp = 1.0;
const static float ki = 1.0;
const static float kd = 1.0;
const static float det = 0.01;
const static float alpha = 1;

/* Shared resources */
extern float v, omega, r_chassis;
extern xSemaphoreHandle posture_mutex;

static pid_state_t left_w = {
	.uk1      = 0,
	.ek1      = 0,
	.ek2      = 0,
	.kp       = kp,
	.ti       = ki * det,
	.td       = kd / det,
	.alpha    = alpha / det,
	.setpoint = 0
};
static pid_state_t right_w = {
	.uk1      = 0,
	.ek1      = 0,
	.ek2      = 0,
	.kp       = kp,
	.ti       = ki * det,
	.td       = kd / det,
	.alpha    = alpha / det,
	.setpoint = 0
};

/* static defined function  */
static pid_out_t pid_loop(pid_state_t *p_pid, float current);
static void pid_loop_task(void);
void pid_update_setpoint(void);

void pid_loop_task_start(unsigned portBASE_TYPE prior)
{
	xTaskCreate((TaskFunction_t)pid_loop_task, "PID_control_loop", 64, (void *) NULL, 
			(UBaseType_t) prior, (TaskHandle_t *) NULL);
	pid_update_setpoint();
}

static void pid_loop_task(void)
{	
	pid_out_t left_w_control, right_w_control;
	float left_current, right_current;
	// TickType_t *pre_loop; /* leave here if latter needs */

	for (;;) {
		// Update pwm value here (using the delayed model in our report)
		(void)left_w_control;
		(void)right_w_control;

		// Wait for IMU task to update this code
		left_current  = 0;
		right_current = 0;
		left_w_control  = pid_loop(&left_w, left_current);
		right_w_control = pid_loop(&right_w, right_current);
		//xTaskDelayUntil(pre_loop, 1);	
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void pid_update_setpoint(void)
{
	float linear, angular;
	if (xSemaphoreTake(posture_mutex, portMAX_DELAY) == pdTRUE) {	// This cause pooling wait
		// retreive data from shared resource
		linear = v;
		angular = omega;
	}
	left_w.setpoint  = linear - r_chassis * angular;
	right_w.setpoint = linear + r_chassis * angular;
}

static pid_out_t pid_loop(pid_state_t *p_pid, float current)
{
	// retreive control params
	float uk1 = p_pid->uk1;
	float ek1 = p_pid->ek1;
	float ek2 = p_pid->ek2;
	float kp  = p_pid->kp;
	float ti  = p_pid->ti;
	float td  = p_pid->td;
	float al  = p_pid->alpha;

	// Compute error at k
	float ek = p_pid->setpoint - current;

	/* PID algorithm, adding anti-windup and low-pass filter by default */
	float up = kp*(ek - ek1);
	float ui = ti * (ek -ek1) / 2;
	/* Anti-windup here, clamping intergral terms 
	 * so that error does not accumulate as the plant saturated
	 */
	ui = (ui < 1) ? ui : 1;
	ui = (ui > -1) ? ui : -1;
	float ud = td * (ek - 2*ek1 + ek2);
	ud = (1 - al)*ek + al * ud;
	float uk = uk1 + up + ui + ud;

	// Update control value
	p_pid->uk1 = uk;
	p_pid->ek2 = ek1;
	p_pid->ek1 = ek;
	
	return (uk >= 0) ? (pid_out_t){uk, 1} : (pid_out_t){-uk, 0};
}

