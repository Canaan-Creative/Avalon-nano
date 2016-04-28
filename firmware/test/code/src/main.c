/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */

#include <string.h>
#include "board.h"
#include "app_usbd_cfg.h"
#ifdef __CODE_RED
#include <NXP/crp.h>
#endif
#include "defines.h"

#include "hid_uart.h"
#include "avalon_a3233.h"
#include "avalon_adc.h"
#include "avalon_led.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif

static void a3233_test(void)
{
	uint8_t freq = 360;
	uint32_t nonce;
	unsigned char golden_ob[] =		"\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";

	led_init();
	uart_init();
	a3233_init();

	while (1) {
		a3233_enable_power(true);
		a3233_reset_asic();
		a3233_set_freq(freq);
		a3233_push_work(golden_ob);
		delay(2000);

		if (!a3222_get_nonce(&nonce)) {
			/* got it */
			if (nonce == (0x187a2 + 0x4000)) {
				/* correct */
				led_rgb(LED_GREEN);
			} else {
				/* wrong */
				led_rgb(LED_BLUE);
			}
		} else {
			/* cann't find it */
			led_rgb(LED_RED);
		}

		a3233_enable_power(false);
		delay(3000);
		led_rgb(LED_BLACK);
	}
}

int main(void)
{
	Board_Init();
	SystemCoreClockUpdate();

	a3233_test();
}

