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
#include "avalon_usb.h"
#include "hid_uart.h"

extern const  USBD_HW_API_T hw_api;
extern const  USBD_CORE_API_T core_api;
extern const  USBD_HID_API_T hid_api;

static USBD_HANDLE_T g_hUsb;
static const  USBD_API_T g_usbApi = {
	&hw_api,
	&core_api,
	0,
	0,
	&hid_api,
	0,
	0,
	0x02221101,
};

const  USBD_API_T *g_pUsbApi = &g_usbApi;

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

/**
 * @brief	Handle interrupt from USB0
 * @return	Nothing
 */
void USB_IRQHandler(void)
{
	USBD_API->hw->ISR(g_hUsb);
}

void usb_init(void)
{
	static bool usbinit = false;
	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;

	if(usbinit)
		return;

	usb_pin_clk_init();

	/* initilize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;
	usb_param.max_num_ep = 2;
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
		ret = hid_init(g_hUsb, (USB_INTERFACE_DESCRIPTOR *) &USB_FsConfigDescriptor[sizeof(USB_CONFIGURATION_DESCRIPTOR)], &usb_param);
		if (ret == LPC_OK) {
			/* Make sure USB and UART IRQ priorities are same for this example */
			NVIC_SetPriority(USB0_IRQn, 1);
			/*  enable USB interrrupts */
			NVIC_EnableIRQ(USB0_IRQn);
			/* now connect */
			USBD_API->hw->Connect(g_hUsb, 1);
		}
	}

	usbinit = true;
}

