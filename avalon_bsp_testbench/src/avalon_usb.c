/*
===============================================================================
 Name        : avalon_usb.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon_usb api
===============================================================================
*/
#include "avalon_api.h"
#include "app_usbd_cfg.h"
#include "cdc_uart.h"

extern const  USBD_HW_API_T hw_api;
extern const  USBD_CORE_API_T core_api;
extern const  USBD_CDC_API_T cdc_api;

static USBD_HANDLE_T g_hUsb;
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

void AVALON_USB_Init(void)
{
	static Bool bUsbInit = FALSE;
	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;


	if(bUsbInit)
		return;

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

	bUsbInit = TRUE;
}
/* Sends a character on the USB */
void AVALON_USB_PutChar(char ch)
{
	UCOM_Write(&ch,1);
	AVALON_Delay(10000);
}

/* Outputs a string on the debug USB */
void AVALON_USB_PutSTR(char *str)
{
	UCOM_Write(str,strlen(str));
	AVALON_Delay(10000);
}

void AVALON_USB_Test(void)
{
	AVALON_USB_Init();

	AVALON_USBPutSTR("hello");
	/* delay for send finish */
	AVALON_Delay(10000);
	AVALON_USBPutChar('V');
}
