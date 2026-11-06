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
#include "hardware_protocol.h"

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
extern volatile uint8_t command;
extern xSemaphoreHandle posture_mutex, velocity_mutex, ref_pos_mutex;

static inline void stable_tracking_loop(float *_vc, float *_omegac,
	const float _xc, const float _xr, const float _yc, const float _yr, 
	const float _thetac, const float _thetar, const float _vr, const float _omegar);

static inline void pose_stabilization_loop(float *_vc, float *_omegac,
	const float _xc, const float _xr, const float _yc, const float _yr, 
	const float _thetac, const float _thetar);

static inline void run_command(const uint8_t cmd,
	const float _xc, const float _xr, const float _yc, const float _yr, 
	const float _thetac, const float _thetar, const float _vr, const float _omegar);

void tracking_loop(void);

void tracking_task_start(unsigned portBASE_TYPE prior)
{
	xTaskCreate((TaskFunction_t)tracking_loop, "stable_tracking_control_loop", 
			512, (void *) NULL, (UBaseType_t) prior, (TaskHandle_t *) NULL);
}

void tracking_loop(void)
{
	float _xc, _xr, _yc, _yr, _thetac, _thetar, _vr, _omegar;
	uint8_t _command;
	TickType_t tick;
	for (;;) {
		tick = xTaskGetTickCount();
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
			_command = command;
			xSemaphoreGive(ref_pos_mutex);
		}
		run_command(_command, _xc, _xr, _yc, _yr, _thetac, _thetar, _vr, _omegar);
		/* run the control loop at discrete time, t = 10ms */
		vTaskDelayUntil(&tick, pdMS_TO_TICKS(10));
	}
}

/* this is separate function for easy to expand if latter command is added */
static inline void run_command(const uint8_t cmd,
	const float _xc, const float _xr, 
	const float _yc, const float _yr, 
	const float _thetac, const float _thetar, 
	const float _vr, const float _omegar)
{
	float _vc, _omegac;
	switch (cmd) {
		case GO_TO_POINT_EXACT:
			/* Interpolation will live in other task, which update the shared resoures */
		case GO_TO_POINT_FAST:
			stable_tracking_loop(&_vc, &_omegac, _xc, _xr, 
					_yc, _yr, _thetac, _thetar, _vr, _omegar);
			break;
		case STABILIZE_AT_POINT:
			pose_stabilization_loop(&_vc, &_omegac, _xc, _xr, 
					_yc, _yr, _thetac, _thetar);
			break;

	}

	/* Update the command value */
	if (xSemaphoreTake(velocity_mutex, portMAX_DELAY) == pdTRUE) {
		vc 	= _vc;
		omegac 	= _omegac;
		xSemaphoreGive(velocity_mutex);
	}
}

/* save some time stacking here with inline, but gcc will do the same job anyway */
static inline void stable_tracking_loop(float *_vc, float *_omegac,
	const float _xc, const float _xr, const float _yc, const float _yr, 
	const float _thetac, const float _thetar, const float _vr, const float _omegar)
{
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
	*_vc 	= kx * xe + _vr * cosf(thetae);
	*_omegac = _omegar + _vr * (ky * ye + kt * sinf(thetae));
}

static inline void pose_stabilization_loop(float *_vc, float *_omegac,
	const float _xc, const float _xr, const float _yc, const float _yr, 
	const float _thetac, const float _thetar)
{
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
	phie = atan2f(ye, xe);
	alphae = thetae - phie;
	float ca, sa, sp;
	ca = cosf(alphae);
	sa = sinf(alphae);
	sp = sinf(phie);

	/* Control rules */
	*_vc 	= - kr * re * ca;
	*_omegac = (sp - sa) * kr * ca - ka * sa;
}
