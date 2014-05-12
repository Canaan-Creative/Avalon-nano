/*
 * @brief USB to UART bridge example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#include <string.h>
#include "board.h"
#include "app_usbd_cfg.h"
#include "cdc_uart.h"
#include "cdc_avalon.h"
#include <NXP/crp.h>
#include "sha2.h"

#define A3233_TASK_LEN 88
#define A3233_NONCE_LEN	4
#define ICA_TASK_LEN 64
#define TICKRATE_HZ2 (2)
#define A3233_TIMER_ICARUS_THRESHOLD	(1)
#define A3233_TIMER_NOICARUS_THRESHOLD	(3)
#define A3233_TEMP_MAX					(60)
#define A3233_TEMP_CHECKINTERVAL		(10)
#define A3233_FREQ_MIN					(100)
#define A3233_FREQ_MAX					(400)

typedef enum{
	A3233_TIMER_ICARUS,
	A3233_TIMER_NOICARUS,
	A3233_TIMER_MAX
}A3233_Timer_e;

__CRP unsigned int CRP_WORD = CRP_NO_ISP;
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

static USBD_HANDLE_T g_hUsb;

extern const  USBD_HW_API_T hw_api;
extern const  USBD_CORE_API_T core_api;
extern const  USBD_CDC_API_T cdc_api;
/* Since this example only uses CDC class link functions for that clas only */
static const  USBD_API_T g_usbApi = {
	&hw_api,
	&core_api,
	0,
	0,
	0,
	&cdc_api,
	0,
	0x02221101,
};

const  USBD_API_T 		*g_pUsbApi = &g_usbApi;
static unsigned char 	golden_ob[] = "\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";
static unsigned int		a3233_curfreq = 400;
static Bool				a3323_ishot = FALSE;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initialize pin and clocks for USB0/USB1 port */
static void usb_pin_clk_init(void)
{
	/* enable USB main clock */
	Chip_Clock_SetUSBClockSource(SYSCTL_USBCLKSRC_PLLOUT, 1);
	/* Enable AHB clock to the USB block and USB RAM. */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USB);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USBRAM);
	/* power UP USB Phy */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPAD_PD);
}

static Bool				a3233_enable = FALSE;
//TODO:add timer api
static unsigned int		timerlist[A3233_TIMER_MAX];

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from USB0
 * @return	Nothing
 */
void USB_IRQHandler(void)
{
	USBD_API->hw->ISR(g_hUsb);
}

/* Find the address of interface descriptor for given class type. */
USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass)
{
	USB_COMMON_DESCRIPTOR *pD;
	USB_INTERFACE_DESCRIPTOR *pIntfDesc = 0;
	uint32_t next_desc_adr;

	pD = (USB_COMMON_DESCRIPTOR *) pDesc;
	next_desc_adr = (uint32_t) pDesc;

	while (pD->bLength) {
		/* is it interface descriptor */
		if (pD->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) {

			pIntfDesc = (USB_INTERFACE_DESCRIPTOR *) pD;
			/* did we find the right interface descriptor */
			if (pIntfDesc->bInterfaceClass == intfClass) {
				break;
			}
		}
		pIntfDesc = 0;
		next_desc_adr = (uint32_t) pD + pD->bLength;
		pD = (USB_COMMON_DESCRIPTOR *) next_desc_adr;
	}

	return pIntfDesc;
}

unsigned int gen_test_a3233(uint32_t *buf)
{
	   buf[0] = 0x11111111;
	   buf[2] = 0x00000000;
	   buf[3] = 0x4ac1d001;
	   buf[4] = 0x89517050;
	   buf[5] = 0x087e051a;
	   buf[6] = 0x06b168ae;
	   buf[7] = 0x62a5f25c;
	   buf[8] = 0x00639107;
	   buf[9] = 0x13cdfd7b;
	   buf[10] = 0xfa77fe7d;
	   buf[11] = 0x9cb18a17;
	   buf[12] = 0x65c90d1e;
	   buf[13] = 0x8f41371d;
	   buf[14] = 0x974bf4bb;
	   buf[15] = 0x7145fd6d;
	   buf[16] = 0xc44192c0;
	   buf[17] = 0x12146495;
	   buf[18] = 0xd8f8ef67;
	   buf[19] = 0xa2cb45c1;
	   buf[20] = 0x00000000;//0x1bee2ba0;
	   buf[21] = 0xaaaaaaaa;
	   return buf[20] + 0x6000;
}

void A3233_CheckTemp() {
	static unsigned int tempcnt = 0;
	static unsigned int checktimes = 1;

	tempcnt++;
	if (tempcnt >= (A3233_TEMP_CHECKINTERVAL * checktimes)) {
		if (a3233_enable) {
			if (tmp102_rd() >= A3233_TEMP_MAX) {
				if (a3233_curfreq > A3233_FREQ_MIN) {
					a3233_curfreq -= 20;
					/* set freq */
					AVALON_POWER_Enable(FALSE);
					a3233_enable = FALSE;
					AVALON_led_rgb(AVALON_LED_OFF);
				} else {
					/* disable a3233 when freq is 20Mhz*/
					AVALON_POWER_Enable(FALSE);
					a3323_ishot = TRUE;
					checktimes = 10;
				}
			} else {
				if (a3233_curfreq <= A3233_FREQ_MAX)
					a3233_curfreq += 20;
				if (a3233_curfreq > A3233_FREQ_MAX)
					a3233_curfreq = A3233_FREQ_MAX;

				AVALON_POWER_Enable(FALSE);
				a3233_enable = FALSE;
				AVALON_led_rgb(AVALON_LED_OFF);
				a3323_ishot = FALSE;
				checktimes = 1;
			}
		}
		tempcnt = 0;
	}

}

/**
 * @brief	Handle interrupt from 32-bit timer
 * @return	Nothing
 */
void TIMER32_0_IRQHandler(void)
{
	A3233_Timer_e	Timer_e;

	if (Chip_TIMER_MatchPending(LPC_TIMER32_0, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER32_0, 1);
		for(Timer_e=A3233_TIMER_ICARUS; Timer_e<A3233_TIMER_MAX; Timer_e++){
			timerlist[Timer_e] ++;
		}
		A3233_CheckTemp();
	}
}

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
int main(void)
{
	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;
	uint8_t 		icarus_buf[ICA_TASK_LEN];
	unsigned int	icarus_buflen = 0;
	unsigned char 	work_buf[A3233_TASK_LEN];
	unsigned char	nonce_buf[A3233_NONCE_LEN];
	unsigned int	nonce_buflen = 0;
	uint32_t 		nonce_value = 0;
	Bool			isgoldenob = FALSE;
	Bool			timestart = FALSE;
	Bool			timer_noicarus = FALSE;
	uint32_t 		timerFreq;
	A3233_Timer_e	Timer_e;

	for(Timer_e=A3233_TIMER_ICARUS; Timer_e<A3233_TIMER_MAX; Timer_e++){
		timerlist[Timer_e] = 0;
	}

	SystemCoreClockUpdate();

	/* enable clocks and pinmux */
	usb_pin_clk_init();

	/* initilize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;
	usb_param.max_num_ep = 3;
	usb_param.mem_base = USB_STACK_MEM_BASE;
	usb_param.mem_size = USB_STACK_MEM_SIZE;

	/* Set the USB descriptors */
	desc.device_desc = (uint8_t *) &USB_DeviceDescriptor[0];
	desc.string_desc = (uint8_t *) &USB_StringDescriptor[0];
	/* Note, to pass USBCV test full-speed only devices should have both
	   descriptor arrays point to same location and device_qualifier set to 0.
	 */
	desc.high_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.full_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.device_qualifier = 0;

	/* USB Initialization */
	ret = USBD_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK) {

		/* Init UCOM - USB to UART bridge interface */
		ret = UCOM_init(g_hUsb, &desc, &usb_param);
		if (ret == LPC_OK) {
			/* Make sure USB and UART IRQ priorities are same for this example */
			NVIC_SetPriority(USB0_IRQn, 1);
			/*  enable USB interrrupts */
			NVIC_EnableIRQ(USB0_IRQn);
			/* now connect */
			USBD_API->hw->Connect(g_hUsb, 1);
		}
	}

	/* Initialize avalon chip */
	AVALON_init();

	/* Enable timer 1 clock */
	Chip_TIMER_Init(LPC_TIMER32_0);

	/* Timer rate is system clock rate */
	timerFreq = Chip_Clock_GetSystemClockRate();

	/* Timer setup for match and interrupt at TICKRATE_HZ */
	Chip_TIMER_Reset(LPC_TIMER32_0);
	Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1);
	Chip_TIMER_SetMatch(LPC_TIMER32_0, 1, (timerFreq / TICKRATE_HZ2));
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_0, 1);

	/* Enable timer interrupt */
	NVIC_ClearPendingIRQ(TIMER_32_0_IRQn);
	NVIC_EnableIRQ(TIMER_32_0_IRQn);

	while (1) {
		while (1) {
			icarus_buflen = UCOM_Read_Cnt();
			if (icarus_buflen > 0) {
				if (timer_noicarus) {
					timer_noicarus = FALSE;
					Chip_TIMER_Disable(LPC_TIMER32_0);
					timerlist[A3233_TIMER_NOICARUS] = 0;
				}

				if(!timestart){
					timerlist[A3233_TIMER_ICARUS] = 0;
					Chip_TIMER_Reset(LPC_TIMER32_0);
					Chip_TIMER_Enable(LPC_TIMER32_0);
					timestart = TRUE;
					continue;
				}

				/* len timeout */
				if(timerlist[A3233_TIMER_ICARUS] >= A3233_TIMER_ICARUS_THRESHOLD){
					timestart = FALSE;

					AVALON_led_rgb(AVALON_LED_OFF);
					AVALON_Delay(10000);
					AVALON_led_rgb(AVALON_LED_GREEN);

					/* clear uart rx ring buffer*/
					UART_FlushRxRB();
					continue;
				}

				if(icarus_buflen >= ICA_TASK_LEN){
					Chip_TIMER_Disable(LPC_TIMER32_0);
					timestart = FALSE;
				}
				else{
					continue;
				}
			}
			else{
				if (!timer_noicarus) {
					timerlist[A3233_TIMER_NOICARUS] = 0;
					Chip_TIMER_Reset(LPC_TIMER32_0);
					Chip_TIMER_Enable(LPC_TIMER32_0);
					timer_noicarus = TRUE;
				}
				if (timerlist[A3233_TIMER_NOICARUS] >= A3233_TIMER_NOICARUS_THRESHOLD) {
					timer_noicarus = FALSE;
					Chip_TIMER_Disable(LPC_TIMER32_0);
					timerlist[A3233_TIMER_NOICARUS] = 0;
					AVALON_POWER_Enable(FALSE);
					a3233_enable = FALSE;
					AVALON_led_rgb(AVALON_LED_OFF);
					AVALON_Delay(10000);
					AVALON_led_rgb(AVALON_LED_GREEN);
				}
				continue;
			}

			AVALON_led_rgb(AVALON_LED_OFF);
			AVALON_Delay(10000);
			AVALON_led_rgb(AVALON_LED_RED);

			memset(icarus_buf, 0, ICA_TASK_LEN);
			UCOM_Read(icarus_buf, ICA_TASK_LEN);
			memset(work_buf, 0, A3233_TASK_LEN);

			if (!memcmp(golden_ob, icarus_buf, ICA_TASK_LEN))
				isgoldenob = TRUE;
			else
				isgoldenob = FALSE;

			data_convert(icarus_buf);
			data_pkg(icarus_buf, work_buf);

			if ((FALSE == a3233_enable) && !a3323_ishot) {
				a3233_enable = TRUE;
				AVALON_POWER_Enable(TRUE);
				AVALON_Rstn_A3233();
				((unsigned int*)work_buf)[1] = AVALON_Gen_A3233_Pll_Cfg(a3233_curfreq, NULL);
			}

			if (isgoldenob) {
				work_buf[81] = 0x1;
				work_buf[82] = 0x73;
				work_buf[83] = 0xa2;
			}

			UART_Write(work_buf, A3233_TASK_LEN);
			/* clear uart rx ring buffer*/
			UART_FlushRxRB();
			break;
		}

		while (1) {
			nonce_buflen = UART_Read_Cnt();
			if (nonce_buflen >= A3233_NONCE_LEN) {
				AVALON_led_rgb(AVALON_LED_OFF);
				AVALON_led_rgb(AVALON_LED_BLUE);

				UART_Read(nonce_buf, A3233_NONCE_LEN);

				PACK32(nonce_buf, &nonce_value);
				nonce_value = ((nonce_value >> 24) | (nonce_value << 24) | ((nonce_value >> 8) & 0xff00) | ((nonce_value << 8) & 0xff0000));
				nonce_value -= 0x1000;
				UNPACK32(nonce_value, nonce_buf);

				UCOM_Write(nonce_buf, A3233_NONCE_LEN);
				break;
			}

			if (UCOM_Read_Cnt())
				break;
			/* Sleep until next IRQ happens */
			__WFI();
		}
	}
}
