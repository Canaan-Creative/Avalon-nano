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
#include "hid_ucom.h"
#include "avalon_api.h"
#include "protocol.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
/* Ring buffer size */
#define BUF_CNT            12
#define UCOM_RX_BUF_SZ      (AVAU_P_COUNT * BUF_CNT)
#define UCOM_TX_BUF_SZ      (AVAU_P_COUNT * BUF_CNT)
#define UCOM_TX_CONNECTED   _BIT(8)
#define UCOM_TX_BUSY        _BIT(0)
#define UCOM_RX_BUF_FULL    _BIT(1)
#define UCOM_RX_BUF_QUEUED  _BIT(2)

static RINGBUFF_T usb_rxrb, usb_txrb;
static uint8_t usb_rxdata[UCOM_RX_BUF_SZ], usb_txdata[UCOM_TX_BUF_SZ];

/**
 * Structure containing Virtual Comm port control data
 */
typedef struct UCOM_DATA {
	USBD_HANDLE_T hUsb;		/*!< Handle to USB stack */
	uint16_t usbRx_count;
	uint8_t *usbTx_buff;
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
static void UCOM_BufInit(void)
{
	RingBuffer_Init(&usb_rxrb, usb_rxdata, 1, UCOM_RX_BUF_SZ);
	RingBuffer_Init(&usb_txrb, usb_txdata, 1, UCOM_TX_BUF_SZ);
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
		if (RingBuffer_GetCount(&usb_txrb) >= AVAU_P_COUNT) {
			g_uCOM.usbTxFlags |= UCOM_TX_BUSY;
			RingBuffer_PopMult(&usb_txrb, g_uCOM.usbTx_buff, AVAU_P_COUNT);
			USBD_API->hw->WriteEP(g_uCOM.hUsb, HID_EP_IN, g_uCOM.usbTx_buff, AVAU_P_COUNT);
		}
		break;

	case USB_EVT_OUT:
		g_uCOM.usbRx_count = USBD_API->hw->ReadEP(hUsb, HID_EP_OUT, g_uCOM.usbRx_buff);
		if (g_uCOM.usbRx_count) {
			if(1 == g_uCOM.usbRx_count)
				RingBuffer_Insert(&usb_rxrb, g_uCOM.usbRx_buff);
			else
				RingBuffer_InsertMult(&usb_rxrb, g_uCOM.usbRx_buff, g_uCOM.usbRx_count);
		}
		if (g_uCOM.usbRxFlags & UCOM_RX_BUF_QUEUED) {
			g_uCOM.usbRxFlags &= ~UCOM_RX_BUF_QUEUED;
			if (g_uCOM.usbRx_count != 0)
				g_uCOM.usbRxFlags |= UCOM_RX_BUF_FULL;
		}
		break;

	case USB_EVT_OUT_NAK:
		/* queue free buffer for RX */
		if ((g_uCOM.usbRxFlags & (UCOM_RX_BUF_FULL | UCOM_RX_BUF_QUEUED)) == 0) {
			g_uCOM.usbRx_count = USBD_API->hw->ReadReqEP(hUsb, HID_EP_OUT, g_uCOM.usbRx_buff, UCOM_RX_BUF_SZ);
			if (g_uCOM.usbRx_count) {
				if(1 == g_uCOM.usbRx_count)
					RingBuffer_Insert(&usb_rxrb, g_uCOM.usbRx_buff);
				else
					RingBuffer_InsertMult(&usb_rxrb, g_uCOM.usbRx_buff, g_uCOM.usbRx_count);
			}
			g_uCOM.usbRxFlags |= UCOM_RX_BUF_QUEUED;
		}
		break;

	default:
		break;
	}

	return LPC_OK;
}

/* USB com port init routine */
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
		g_uCOM.usbRx_buff = (uint8_t *)hid_param.mem_base;
		hid_param.mem_base += UCOM_RX_BUF_SZ;
		hid_param.mem_size -= UCOM_RX_BUF_SZ;

		g_uCOM.usbTx_buff = (uint8_t *)hid_param.mem_base;
		hid_param.mem_base += UCOM_TX_BUF_SZ;
		hid_param.mem_size -= UCOM_TX_BUF_SZ;

		UCOM_BufInit();

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

	if(1 == len)
		RingBuffer_Insert(&usb_txrb, pBuf);
	else
		RingBuffer_InsertMult(&usb_txrb, pBuf, len);

	if (g_uCOM.usbTxFlags & UCOM_TX_CONNECTED) {
		if (!(g_uCOM.usbTxFlags & UCOM_TX_BUSY) && (RingBuffer_GetCount(&usb_txrb) >= AVAU_P_COUNT)) {
			g_uCOM.usbTxFlags |= UCOM_TX_BUSY;
			RingBuffer_PopMult(&usb_txrb, g_uCOM.usbTx_buff, AVAU_P_COUNT);
			ret = USBD_API->hw->WriteEP(g_uCOM.hUsb, HID_EP_IN, g_uCOM.usbTx_buff, AVAU_P_COUNT);
		}
	}

	return ret;
}

/* clear UCOM rx ringbuffer */
void UCOM_FlushRxRB(void)
{
	RingBuffer_Flush(&usb_rxrb);
}

