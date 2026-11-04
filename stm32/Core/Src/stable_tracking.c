/**
 * @file    : stable_tracking.c
 * @brief   : implementation of Kanayama et al. stable tracking controller.
 * Gains was tunned and shown in the report.
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

#include <math.h>
#include "FreeRTOS.h"
#include "semphr.h"

const static float kx = 2.0; 
const static float ky = 100.0; 
const static float kt = 20.0; 

static float xe, ye,thetae;
extern float xr, yr, thetar, vr, omegar;	// command value from ESP32, and Interpolator
extern float xc, yc, thetac, vc, omegac;	// current value
extern xSemaphoreHandle posture_mutex, velocity_mutex, ref_pos_mutex;

void stable_tracking_loop(void);

void stable_tracking_task_start(unsigned portBASE_TYPE prior)
{
	xTaskCreate((TaskFunction_t)stable_tracking_loop, "stable_tracking_control_loop", 
			512, (void *) NULL, (UBaseType_t) prior, (TaskHandle_t *) NULL);
}

void stable_tracking_loop(void)
{
	float _xc, _xr, _yc, _yr, _thetac, _thetar, _vc, _vr, _omegac, _omegar;
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
			xSemaphoreGive(ref_pos_mutex);
		}
		/* Compute error */
		xe = cos(_thetac) * (_xr - _xc) + sin(_thetac) * (_yr - _yc);
		ye = cos(_thetac) * (_yr - _yc) - sin(_thetac) * (_xr - _xc);
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
		/* run the control loop at discrete time, t = 10ms */
		vTaskDelayUntil(&tick, pdMS_TO_TICKS(10));
	}
}
