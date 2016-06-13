/*
 * @brief
 *
 * @note
 * Author: fanzixiao@canaan-creative.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */

#include "board.h"
#include "avalon_fan.h"
#include "avalon_timer.h"

#define SYS_CLOCK_FRE      48000000
#define FAN_PWM_STD_FRE    50000
#define FAN_PWM_DUTY_BASE  256.0f
#define FAN_PWM_DUTY       0
#define FAN_PWM_POINT      0
#define FAN_PWM_PIN        14

#define FAN_SPEED_POINT    0
#define FAN_SPEED_PIN      0

#define FAN_PWM_CYCLE (SYS_CLOCK_FRE / FAN_PWM_STD_FRE)

#define TIME_FAN_SPEED_SAMPLE_CYCLE	2000
#define TIME_FAN_SPEED_SAMPLE_SUM	30

static uint32_t fan_speed = 0;
static uint32_t fan_speed_pluse_cnt = 0;
static bool fan_speed_pluse_change = false;

void fan_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, FAN_SPEED_POINT, FAN_SPEED_PIN, (IOCON_FUNC1 | IOCON_MODE_PULLUP));
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, FAN_SPEED_POINT, FAN_SPEED_PIN);
	timer_set(TIMER_ID2, TIME_FAN_SPEED_SAMPLE_CYCLE);

	Chip_IOCON_PinMuxSet(LPC_IOCON, FAN_PWM_POINT, FAN_PWM_PIN, (IOCON_FUNC3 | IOCON_MODE_INACT));
	Chip_TIMER_Init(LPC_TIMER32_1);
	Chip_TIMER_Disable(LPC_TIMER32_1);

	Chip_TIMER_ClearMatch(LPC_TIMER32_1, 1);
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER32_1, 0, TIMER_EXTMATCH_SET, 1);
	Chip_TIMER_SetMatch(LPC_TIMER32_1, 1, FAN_PWM_DUTY);

	Chip_TIMER_PrescaleSet(LPC_TIMER32_1, 0);
	Chip_TIMER_SetMatch(LPC_TIMER32_1, 0, FAN_PWM_CYCLE);
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_1, 0);
	LPC_TIMER32_1->PWMC = 0x02;

	Chip_TIMER_Enable(LPC_TIMER32_1);
}


void fan_enable(void)
{
	Chip_TIMER_Enable(LPC_TIMER32_1);
}

void fan_disable(void)
{
	Chip_TIMER_Disable(LPC_TIMER32_1);
}

void fan_adjust_pwm(uint8_t pwm_duty)
{
	uint32_t tmp_fan_pwm_duty;

	tmp_fan_pwm_duty = FAN_PWM_CYCLE / FAN_PWM_DUTY_BASE * pwm_duty;

	Chip_TIMER_Disable(LPC_TIMER32_1);
	Chip_TIMER_Reset(LPC_TIMER32_1);

	Chip_TIMER_ClearMatch(LPC_TIMER32_1, 1);
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER32_1, 0, TIMER_EXTMATCH_SET, 1);
	Chip_TIMER_SetMatch(LPC_TIMER32_1, 1, tmp_fan_pwm_duty);

	Chip_TIMER_Enable(LPC_TIMER32_1);
}

void fan_measure_speed(void)
{
	if (Chip_GPIO_GetPinState(LPC_GPIO, FAN_SPEED_POINT, FAN_SPEED_PIN)) {
		if (fan_speed_pluse_change == false) {
			fan_speed_pluse_change = true;
			fan_speed_pluse_cnt++;
		}
	} else 
		fan_speed_pluse_change = false;

	if (timer_istimeout(TIMER_ID2)) {
		timer_set(TIMER_ID2, TIME_FAN_SPEED_SAMPLE_CYCLE);
		fan_speed = fan_speed_pluse_cnt * TIME_FAN_SPEED_SAMPLE_SUM;
		fan_speed_pluse_cnt = 0;
	}
}

uint32_t fan_get_speed(void)
{
	return fan_speed;
}
