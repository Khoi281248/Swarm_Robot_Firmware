/**
 * @file    : stable_tracking.c
 * @brief   : implementation of Kanayama et al. stable tracking controller.
 * And Pose stabilization controller.
 * Gains was tunned and shown in the report.
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#include <math.h>
#include "app_tasks.h"

/* Stable tracking gains */
const static float kx = 2.0f; 
const static float ky = 100.0f; 
const static float kt = 20.0f;
/* Pose stabilization gains */
const static float kr = 2.0f; 
const static float ka = 4.0f;


static float xe, ye,thetae;
extern volatile float xr, yr, thetar, vr, omegar;	// command value from ESP32, and Interpolator
extern volatile float xc, yc, thetac, vc, omegac;	// current value
extern xSemaphoreHandle posture_mutex, velocity_mutex, ref_pos_mutex;

static void stable_tracking_loop(void);
static void pose_stabilization_loop(void);
void tracking_loop(void);

void tracking_task_start(unsigned portBASE_TYPE prior)
{
	xTaskCreate((TaskFunction_t)tracking_loop, "stable_tracking_control_loop", 
			512, (void *) NULL, (UBaseType_t) prior, (TaskHandle_t *) NULL);
}

void tracking_loop(void)
{
	TickType_t tick;
	for (;;) {
		tick = xTaskGetTickCount();
		/* run the control loop at discrete time, t = 10ms */
		vTaskDelayUntil(&tick, pdMS_TO_TICKS(10));
	}
}

static void stable_tracking_loop(void)
{
	float _xc, _xr, _yc, _yr, _thetac, _thetar, _vc, _vr, _omegac, _omegar;
	/* copy shared resource */
	if (xSemaphoreTake(posture_mutex, portMAX_DELAY) == pdTRUE) {
		_xc = xc;
		_yc = yc;
		_thetac = thetac;
		xSemaphoreGive(posture_mutex);
	} if (xSemaphoreTake(ref_pos_mutex, portMAX_DELAY) == pdTRUE) {
		_xr = xr;
		_yr = yr;
		_thetar = thetar;
		_vr = vr;
		_omegar = omegar;
		xSemaphoreGive(ref_pos_mutex);
	}
	/* Compute error */
	float dx, dy, ct, st;
	dx = _xr - _xc;
	dy = _yr - _yc;
	ct = cosf(_thetac);
	st = sinf(_thetac);

	xe = ct * dx + st * dy;
	ye = ct * dy - st * dx;
	thetae = _thetar - _thetac;
	/* Control rules */
	_vc 	= kx * xe + _vr * cos(thetae);
	_omegac = _omegar + _vr * (ky * ye + kt * sin(thetae));

	/* Update the command value */
	if (xSemaphoreTake(velocity_mutex, portMAX_DELAY) == pdTRUE) {
		vc = _vc;
		omegac = _omegac;
		xSemaphoreGive(velocity_mutex);
	}
}

static void pose_stabilization_loop(void) 
{
	float _xc, _xr, _yc, _yr, _thetac, _thetar, _vc, _omegac;
	/* copy shared resource */
	if (xSemaphoreTake(posture_mutex, portMAX_DELAY) == pdTRUE) {
		_xc = xc;
		_yc = yc;
		_thetac = thetac;
		xSemaphoreGive(posture_mutex);
	} if (xSemaphoreTake(ref_pos_mutex, portMAX_DELAY) == pdTRUE) {
		_xr = xr;
		_yr = yr;
		_thetar = thetar;
		xSemaphoreGive(ref_pos_mutex);
	}
	/* Compute error */
	float dx, dy, ct, st;
	dx = _xc - _xr;
	dy = _yc - _yr;
	ct = cosf(_thetar);
	st = sinf(_thetar);

	xe = ct * dx + st * dy;
	ye = ct * dy - st * dx;
	thetae = _thetac - _thetar;

	/* Convert to Polar coordinate */
	float re, phie, alphae;
	re = sqrt(pow(xe, 2) + pow(ye, 2));
	phie = atan2(ye, xe);
	alphae = thetae - phie;
	float ca, sa, sp;
	ca = cos(alphae);
	sa = sin(alphae);
	sp = sin(phie);

	/* Control rules */
	_vc 	= - kr * re * ca;
	_omegac = (sp - sa) * kr * ca - ka * sa;

	/* Update the command value */
	if (xSemaphoreTake(velocity_mutex, portMAX_DELAY) == pdTRUE) {
		vc = _vc;
		omegac = _omegac;
		xSemaphoreGive(velocity_mutex);
	}
}
