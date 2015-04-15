/*
 * @brief UART Comm port call back routines
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
#include "hid_uart.h"
#include "avalon_api.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Ring buffer size */
#define UCOM_RX_BUF_SZ		512
#define UART_RX_BUF_SZ		128
#define UART_TX_BUF_SZ		512
#define UCOM_TX_CONNECTED   _BIT(8)		/* connection state is for both RX/Tx */
#define UCOM_TX_BUSY        _BIT(0)
#define UCOM_RX_BUF_FULL    _BIT(1)
#define UCOM_RX_BUF_QUEUED  _BIT(2)
#define UCOM_RX_DB_QUEUED   _BIT(3)
#define UCOM_REPORT_SIZE     64
/*
 * uart only use rx ringbuf, tx use send block
 * usb only use rx ringbuf, tx use write ep
 * */
STATIC RINGBUFF_T uart_rxrb, uart_txrb;
STATIC RINGBUFF_T usb_rxrb;

static uint8_t uart_rxbuff[UART_RX_BUF_SZ], uart_txbuff[UART_TX_BUF_SZ];
static uint8_t usb_rxbuff[UCOM_RX_BUF_SZ];

/**
 * Structure containing Virtual Comm port control data
 */
typedef struct UCOM_DATA {
	USBD_HANDLE_T hUsb;		/*!< Handle to USB stack */
	uint16_t usbRx_count;
	uint8_t *usbRx_buff;
	volatile uint16_t usbTxFlags;	/*!< USB Tx Flag */
	volatile uint16_t usbRxFlags;	/*!< USB Rx Flag */
} UCOM_DATA_T;

/** Virtual Comm port control data instance. */
static UCOM_DATA_T g_uCOM;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
extern const uint8_t UCOM_ReportDescriptor[];
extern const uint16_t UCOM_ReportDescSize;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static void Init_UART_PinMux(void)
{
#if (defined(BOARD_NXP_XPRESSO_11U14) || defined(BOARD_NGX_BLUEBOARD_11U24))
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_MODE_PULLUP);	/* PIO0_18 used for RXD */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 19, IOCON_FUNC1 | IOCON_MODE_INACT);	/* PIO0_19 used for TXD */
#else
#error "No Pin muxing defined for UART operation"
#endif
}

/* UART port init routine */
static void UCOM_UartInit(void)
{
	/* Board specific muxing */
	Init_UART_PinMux();

	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaudFDR(LPC_USART, 111111);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);

	RingBuffer_Init(&uart_rxrb, uart_rxbuff, 1, UART_RX_BUF_SZ);
	RingBuffer_Init(&uart_txrb, uart_txbuff, 1, UART_TX_BUF_SZ);
	RingBuffer_Init(&usb_rxrb, usb_rxbuff, 1, UCOM_RX_BUF_SZ);

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* Enable Interrupt for UART channel */
	/* Priority = 1 */
	NVIC_SetPriority(UART0_IRQn, 1);
	/* Enable Interrupt for UART channel */
	NVIC_EnableIRQ(UART0_IRQn);

	g_uCOM.usbTxFlags |= UCOM_TX_CONNECTED;
}

/* HID Get Report Request Callback. Called automatically on HID Get Report Request */
static ErrorCode_t UCOM_GetReport(USBD_HANDLE_T hHid, USB_SETUP_PACKET *pSetup, uint8_t * *pBuffer, uint16_t *plength)
{
	return LPC_OK;
}

/* HID Set Report Request Callback. Called automatically on HID Set Report Request */
static ErrorCode_t UCOM_SetReport(USBD_HANDLE_T hHid, USB_SETUP_PACKET *pSetup, uint8_t * *pBuffer, uint16_t length)
{
	return LPC_OK;
}

/* UCOM interrupt EP_IN and EP_OUT endpoints handler */
static ErrorCode_t UCOM_int_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	switch (event) {
	case USB_EVT_IN:
		/* USB_EVT_IN occurs when HW completes sending IN packet. So clear the
		    busy flag for main loop to queue next packet.
		 */
		g_uCOM.usbTxFlags &= ~UCOM_TX_BUSY;
		break;

	case USB_EVT_OUT:
		g_uCOM.usbRx_count = USBD_API->hw->ReadEP(hUsb, HID_EP_OUT, g_uCOM.usbRx_buff);
		if(g_uCOM.usbRx_count){
			if(1 == g_uCOM.usbRx_count){
				RingBuffer_Insert(&usb_rxrb, g_uCOM.usbRx_buff);
			}else
			{
				RingBuffer_InsertMult(&usb_rxrb, g_uCOM.usbRx_buff, g_uCOM.usbRx_count);
			}
		}
		if (g_uCOM.usbRxFlags & UCOM_RX_BUF_QUEUED) {
			g_uCOM.usbRxFlags &= ~UCOM_RX_BUF_QUEUED;
			if (g_uCOM.usbRx_count != 0) {
				g_uCOM.usbRxFlags |= UCOM_RX_BUF_FULL;
			}
		}
		break;

	case USB_EVT_OUT_NAK:
		/* queue free buffer for RX */
		if ((g_uCOM.usbRxFlags & (UCOM_RX_BUF_FULL | UCOM_RX_BUF_QUEUED)) == 0) {
			g_uCOM.usbRx_count = USBD_API->hw->ReadReqEP(hUsb, HID_EP_OUT, g_uCOM.usbRx_buff, UCOM_RX_BUF_SZ);
			if(g_uCOM.usbRx_count){
				if(1 == g_uCOM.usbRx_count){
					RingBuffer_Insert(&usb_rxrb, g_uCOM.usbRx_buff);
				}else
				{
					RingBuffer_InsertMult(&usb_rxrb, g_uCOM.usbRx_buff, g_uCOM.usbRx_count);
				}
			}
			g_uCOM.usbRxFlags |= UCOM_RX_BUF_QUEUED;
		}
		break;

	default:
		break;
	}

	return LPC_OK;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	UART interrupt handler sub-routine
 * @return	Nothing
 */
void UART_IRQHandler(void)
{
	Chip_UART_IRQRBHandler(LPC_USART, &uart_rxrb, &uart_txrb);
}

/* UART to USB com port init routine */
ErrorCode_t UCOM_init(USBD_HANDLE_T hUsb, USB_INTERFACE_DESCRIPTOR *pIntfDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_HID_INIT_PARAM_T hid_param;
	USB_HID_REPORT_T reports_data[1];
	ErrorCode_t ret = LPC_OK;

	/* Store USB stack handle for future use. */
	g_uCOM.hUsb = hUsb;
	/* Initi CDC params */
	memset((void *) &hid_param, 0, sizeof(USBD_HID_INIT_PARAM_T));
	hid_param.max_reports = 1;
	hid_param.mem_base = pUsbParam->mem_base;
	hid_param.mem_size = pUsbParam->mem_size;
	hid_param.intf_desc = (uint8_t *)pIntfDesc;
	hid_param.HID_GetReport = UCOM_GetReport;
	hid_param.HID_SetReport = UCOM_SetReport;
	hid_param.HID_EpIn_Hdlr = UCOM_int_hdlr;
	hid_param.HID_EpOut_Hdlr = UCOM_int_hdlr;
	reports_data[0].len = UCOM_ReportDescSize;
	reports_data[0].idle_time = 0;
	reports_data[0].desc = (uint8_t *) &UCOM_ReportDescriptor[0];
	hid_param.report_data  = reports_data;

	/* Init HID interface */
	ret = USBD_API->hid->init(hUsb, &hid_param);

	if (ret == LPC_OK) {
		/* allocate transfer buffers */
		g_uCOM.usbRx_buff = (uint8_t *) hid_param.mem_base;
		hid_param.mem_base += UCOM_RX_BUF_SZ;
		hid_param.mem_size -= UCOM_RX_BUF_SZ;

		/* Init UART port for bridging */
		UCOM_UartInit();

		/* update mem_base and size variables for cascading calls. */
		pUsbParam->mem_base = hid_param.mem_base;
		pUsbParam->mem_size = hid_param.mem_size;
	}

	return ret;
}

/* Gets current read count. */
uint32_t UCOM_Read_Cnt(void)
{
	return RingBuffer_GetCount(&usb_rxrb);
}

/* Read data from usb */
uint32_t UCOM_Read(uint8_t *pBuf, uint32_t buf_len)
{
	uint16_t cnt = 0;

	cnt = RingBuffer_PopMult(&usb_rxrb, (uint8_t *) pBuf, buf_len);
	g_uCOM.usbRxFlags &= ~UCOM_RX_BUF_FULL;

	return cnt;
}

/* Send data to usb */
uint32_t UCOM_Write(uint8_t *pBuf, uint32_t len)
{
	uint32_t ret = 0;
	unsigned int timeout;

	if (g_uCOM.usbTxFlags & UCOM_TX_CONNECTED) {
		timeout = 0;
		while ((g_uCOM.usbTxFlags & UCOM_TX_BUSY) == 1) {
			AVALON_Delay(20);
			timeout ++;
			/*FIXME: busy is not always right,we must send after timeout */
			if (timeout > 3)
				break;
		}

		g_uCOM.usbTxFlags |= UCOM_TX_BUSY;
		ret = USBD_API->hw->WriteEP(g_uCOM.hUsb, HID_EP_IN, pBuf, len);
	}

	return ret;
}

/* clear UCOM rx ringbuffer */
void UCOM_FlushRxRB(void)
{
	RingBuffer_Flush(&usb_rxrb);
}

/* Gets current read count. */
uint32_t UART_Read_Cnt(void)
{
	return RingBuffer_GetCount(&uart_rxrb);
}


/* Read data from uart */
uint32_t UART_Read(uint8_t *pBuf, uint32_t buf_len)
{
	uint16_t cnt = 0;

	if(pBuf)
	{
		cnt = Chip_UART_ReadRB(LPC_USART, &uart_rxrb, pBuf, buf_len);
	}

	return cnt;
}

/* Send data to uart */
uint32_t UART_Write(uint8_t *pBuf, uint32_t len)
{
	uint32_t ret = 0;

	if(pBuf)
	{
		ret = Chip_UART_SendRB(LPC_USART, &uart_txrb, pBuf, len);
	}

	return ret;
}

/* clear UART tx ringbuffer */
void UART_FlushTxRB(void)
{
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2 | UART_FCR_TX_RS));
	RingBuffer_Flush(&uart_txrb);
}

/* clear UART rx ringbuffer */
void UART_FlushRxRB(void)
{
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2 | UART_FCR_RX_RS));
	RingBuffer_Flush(&uart_rxrb);
}
