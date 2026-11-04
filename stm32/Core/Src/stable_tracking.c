/**
 * @file    : stable_tracking.c
 * @brief   : implementation of Kanayama et al. stable tracking controller.
 * Gains was tunned and shown in the report.
 *
 * @Author  : Loiturong
 * @License : GNU GENERAL PUBLIC LICENSE
 */

static float xe, ye,thetae;
extern float xr, yr, thetar, vr, omegar;	// command value from ESP32, and Interpolator
extern float xc, yc, thetac, vc, omegac;	// current value

void stable_tracking_task_start(unsigned portBASE_TYPE prior)
{
	
}
