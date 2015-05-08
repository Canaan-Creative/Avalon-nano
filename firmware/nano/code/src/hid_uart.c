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

#include "protocol.h"
#include "hid_uart.h"

/* Ring buffer size */
#define BUF_CNT             4
#define UCOM_RX_BUF_SZ      (AVAU_P_COUNT * BUF_CNT)
#define UCOM_TX_BUF_SZ      (AVAU_P_COUNT * BUF_CNT)
#define UART_RX_BUF_SZ      (A3233_NONCE_LEN * BUF_CNT)
#define UART_TX_BUF_SZ      (A3233_TASK_LEN * BUF_CNT)
#define UCOM_TX_CONNECTED   _BIT(8)
#define UCOM_TX_BUSY        _BIT(0)
#define UCOM_RX_BUF_FULL    _BIT(1)
#define UCOM_RX_BUF_QUEUED  _BIT(2)
#define UCOM_RX_DB_QUEUED   _BIT(3)

struct ucom_ctx {
	USBD_HANDLE_T usb_h;		/*!< Handle to USB stack */
	uint16_t usbrx_count;
	uint8_t *usbtx_buff;
	uint8_t *usbrx_buff;
	volatile uint16_t usbtxflags;	/*!< USB Tx Flag */
	volatile uint16_t usbrxflags;	/*!< USB Rx Flag */
};

static RINGBUFF_T uart_rxrb, uart_txrb;
static RINGBUFF_T usb_rxrb, usb_txrb;

static uint8_t uart_rxdata[UART_RX_BUF_SZ], uart_txdata[UART_TX_BUF_SZ];
static uint8_t usb_rxdata[UCOM_RX_BUF_SZ], usb_txdata[UCOM_TX_BUF_SZ];

static ucom_ctx g_ucom;

extern const uint8_t hid_reportdescriptor[];
extern const uint16_t hid_reportdescsize;

static void init_uart_pinmux(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_MODE_PULLUP);	/* PIO0_18 used for RXD */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 19, IOCON_FUNC1 | IOCON_MODE_INACT);	/* PIO0_19 used for TXD */
}

/* UART port init routine */
static void uart_init(void)
{
	/* Board specific muxing */
	init_uart_pinmux();

	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaudFDR(LPC_USART, 111111);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);

	RingBuffer_Init(&uart_rxrb, uart_rxdata, 1, UART_RX_BUF_SZ);
	RingBuffer_Init(&uart_txrb, uart_txdata, 1, UART_TX_BUF_SZ);
	RingBuffer_Init(&usb_rxrb, usb_rxdata, 1, UCOM_RX_BUF_SZ);
	RingBuffer_Init(&usb_txrb, usb_txdata, 1, UCOM_TX_BUF_SZ);

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* Enable Interrupt for UART channel */
	/* Priority = 1 */
	NVIC_SetPriority(UART0_IRQn, 1);
	/* Enable Interrupt for UART channel */
	NVIC_EnableIRQ(UART0_IRQn);

	g_ucom.usbtxflags |= UCOM_TX_CONNECTED;
}

/* HID Get Report Request Callback. Called automatically on HID Get Report Request */
static ErrorCode_t hid_getreport(USBD_HANDLE_T hHid, USB_SETUP_PACKET *pSetup, uint8_t * *pBuffer, uint16_t *plength)
{
	return LPC_OK;
}

/* HID Set Report Request Callback. Called automatically on HID Set Report Request */
static ErrorCode_t hid_setreport(USBD_HANDLE_T hHid, USB_SETUP_PACKET *pSetup, uint8_t * *pBuffer, uint16_t length)
{
	return LPC_OK;
}

/* UCOM interrupt EP_IN and EP_OUT endpoints handler */
static ErrorCode_t hid_int_hdlr(USBD_HANDLE_T usb_h, void *data, uint32_t event)
{
	switch (event) {
	case USB_EVT_IN:
		/* USB_EVT_IN occurs when HW completes sending IN packet. So clear the
		    busy flag for main loop to queue next packet.
		 */
		g_ucom.usbtxflags &= ~UCOM_TX_BUSY;
		if (RingBuffer_GetCount(&usb_txrb) >= AVAU_P_COUNT) {
			g_ucom.usbtxflags |= UCOM_TX_BUSY;
			RingBuffer_PopMult(&usb_txrb, g_ucom.usbtx_buff, AVAU_P_COUNT);
			USBD_API->hw->WriteEP(g_ucom.usb_h, HID_EP_IN, g_ucom.usbtx_buff, AVAU_P_COUNT);
		}
		break;

	case USB_EVT_OUT:
		g_ucom.usbrx_count = USBD_API->hw->ReadEP(usb_h, HID_EP_OUT, g_ucom.usbrx_buff);
		if (g_ucom.usbrx_count) {
			if(1 == g_ucom.usbrx_count)
				RingBuffer_Insert(&usb_rxrb, g_ucom.usbrx_buff);
			else
				RingBuffer_InsertMult(&usb_rxrb, g_ucom.usbrx_buff, g_ucom.usbrx_count);
		}
		if (g_ucom.usbrxflags & UCOM_RX_BUF_QUEUED) {
			g_ucom.usbrxflags &= ~UCOM_RX_BUF_QUEUED;
			if (g_ucom.usbrx_count != 0)
				g_ucom.usbrxflags |= UCOM_RX_BUF_FULL;
		}
		break;

	case USB_EVT_OUT_NAK:
		/* queue free buffer for RX */
		if ((g_ucom.usbrxflags & (UCOM_RX_BUF_FULL | UCOM_RX_BUF_QUEUED)) == 0) {
			g_ucom.usbrx_count = USBD_API->hw->ReadReqEP(usb_h, HID_EP_OUT, g_ucom.usbrx_buff, UCOM_RX_BUF_SZ);
			if (g_ucom.usbrx_count) {
				if(1 == g_ucom.usbrx_count)
					RingBuffer_Insert(&usb_rxrb, g_ucom.usbrx_buff);
				else
					RingBuffer_InsertMult(&usb_rxrb, g_ucom.usbrx_buff, g_ucom.usbrx_count);
			}
			g_ucom.usbrxflags |= UCOM_RX_BUF_QUEUED;
		}
		break;

	default:
		break;
	}

	return LPC_OK;
}

void UART_IRQHandler(void)
{
	Chip_UART_IRQRBHandler(LPC_USART, &uart_rxrb, &uart_txrb);
}

int hid_init(USBD_HANDLE_T usb_h, USB_INTERFACE_DESCRIPTOR *pIntfDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_HID_INIT_PARAM_T hid_param;
	USB_HID_REPORT_T reports_data[1];
	ErrorCode_t ret = LPC_OK;

	/* Store USB stack handle for future use. */
	g_ucom.usb_h = usb_h;
	/* Init hid params */
	memset((void *) &hid_param, 0, sizeof(USBD_HID_INIT_PARAM_T));
	hid_param.max_reports = 1;
	hid_param.mem_base = pUsbParam->mem_base;
	hid_param.mem_size = pUsbParam->mem_size;
	hid_param.intf_desc = (uint8_t *)pIntfDesc;
	hid_param.HID_GetReport = hid_getreport;
	hid_param.HID_SetReport = hid_setreport;
	hid_param.HID_EpIn_Hdlr = hid_int_hdlr;
	hid_param.HID_EpOut_Hdlr = hid_int_hdlr;
	reports_data[0].len = hid_reportdescsize;
	reports_data[0].idle_time = 0;
	reports_data[0].desc = (uint8_t *) &hid_reportdescriptor[0];
	hid_param.report_data  = reports_data;

	/* Init HID interface */
	ret = USBD_API->hid->init(usb_h, &hid_param);

	if (ret == LPC_OK) {
		/* allocate transfer buffers */
		g_ucom.usbrx_buff = (uint8_t *)hid_param.mem_base;
		hid_param.mem_base += UCOM_RX_BUF_SZ;
		hid_param.mem_size -= UCOM_RX_BUF_SZ;

		g_ucom.usbtx_buff = (uint8_t *)hid_param.mem_base;
		hid_param.mem_base += UCOM_TX_BUF_SZ;
		hid_param.mem_size -= UCOM_TX_BUF_SZ;

		/* Init UART port for bridging */
		uart_init();

		/* update mem_base and size variables for cascading calls. */
		pUsbParam->mem_base = hid_param.mem_base;
		pUsbParam->mem_size = hid_param.mem_size;
		return 0;
	}

	return 1;
}

/* Get package counts from usb rx buf. */
uint32_t hid_rxrb_cnt(void)
{
	return RingBuffer_GetCount(&usb_rxrb);
}

/* Read data from usb */
uint32_t hid_read(uint8_t *pbuf, uint32_t buf_len)
{
	uint16_t cnt = 0;

	cnt = RingBuffer_PopMult(&usb_rxrb, (uint8_t *) pbuf, buf_len);
	g_ucom.usbrxflags &= ~UCOM_RX_BUF_FULL;

	return cnt;
}

/* Send data to usb */
uint32_t hid_write(uint8_t *pbuf, uint32_t len)
{
	uint32_t ret = 0;

	if(1 == len)
		RingBuffer_Insert(&usb_txrb, pbuf);
	else
		RingBuffer_InsertMult(&usb_txrb, pbuf, len);

	if (g_ucom.usbtxflags & UCOM_TX_CONNECTED) {
		if (!(g_ucom.usbtxflags & UCOM_TX_BUSY) && (RingBuffer_GetCount(&usb_txrb) >= AVAU_P_COUNT)) {
			g_ucom.usbtxflags |= UCOM_TX_BUSY;
			RingBuffer_PopMult(&usb_txrb, g_ucom.usbtx_buff, AVAU_P_COUNT);
			ret = USBD_API->hw->WriteEP(g_ucom.usb_h, HID_EP_IN, g_ucom.usbtx_buff, AVAU_P_COUNT);
		}
	}

	return ret;
}

/* clear UCOM rx ringbuffer */
void hid_flush_rxrb(void)
{
	RingBuffer_Flush(&usb_rxrb);
}

/* Gets current read count. */
uint32_t uart_rxrb_cnt(void)
{
	return RingBuffer_GetCount(&uart_rxrb);
}

/* Read data from uart */
uint32_t uart_read(uint8_t *pbuf, uint32_t buf_len)
{
	uint16_t cnt = 0;

	if(pbuf)
	{
		cnt = Chip_UART_ReadRB(LPC_USART, &uart_rxrb, pbuf, buf_len);
	}

	return cnt;
}

/* Send data to uart */
uint32_t uart_write(uint8_t *pbuf, uint32_t len)
{
	uint32_t ret = 0;

	if(pbuf)
	{
		ret = Chip_UART_SendRB(LPC_USART, &uart_txrb, pbuf, len);
	}

	return ret;
}

/* clear UART tx ringbuffer */
void uart_flush_txrb(void)
{
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2 | UART_FCR_TX_RS));
	RingBuffer_Flush(&uart_txrb);
}

/* clear UART rx ringbuffer */
void uart_flushrxrb(void)
{
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2 | UART_FCR_RX_RS));
	RingBuffer_Flush(&uart_rxrb);
}

