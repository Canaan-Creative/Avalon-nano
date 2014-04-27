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
#include "cdc_uart.h"
#include "cdc_avalon.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Ring buffer size */
#define UCOM_RX_BUF_SZ      1024
#define UART_RX_BUF_SZ		128
#define UART_TX_BUF_SZ		1024
#define UCOM_TX_CONNECTED   _BIT(8)		/* connection state is for both RX/Tx */
#define UCOM_TX_BUSY        _BIT(0)
#define UCOM_RX_BUF_FULL    _BIT(1)
#define UCOM_RX_BUF_QUEUED  _BIT(2)
#define UCOM_RX_DB_QUEUED   _BIT(3)

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
	USBD_HANDLE_T hCdc;		/*!< Handle to CDC class controller */

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
	Chip_UART_SetBaudFDR(LPC_USART, 57600);
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
}

/* UCOM bulk EP_IN and EP_OUT endpoints handler */
static ErrorCode_t UCOM_bulk_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	UCOM_DATA_T *pUcom = (UCOM_DATA_T *) data;

	switch (event) {
	/* A transfer from us to the USB host that we queued has completed. */
	case USB_EVT_IN:
		pUcom->usbTxFlags &= ~UCOM_TX_BUSY;
		break;

	/* We received a transfer from the USB host . */
	case USB_EVT_OUT:
		pUcom->usbRx_count = USBD_API->hw->ReadEP(hUsb, USB_CDC_OUT_EP, pUcom->usbRx_buff);
		if(pUcom->usbRx_count){
			if(1 == pUcom->usbRx_count){
				RingBuffer_Insert(&usb_rxrb, pUcom->usbRx_buff);
			}else
			{
				RingBuffer_InsertMult(&usb_rxrb, pUcom->usbRx_buff, pUcom->usbRx_count);
			}
		}
		if (pUcom->usbRxFlags & UCOM_RX_BUF_QUEUED) {
			pUcom->usbRxFlags &= ~UCOM_RX_BUF_QUEUED;
			if (pUcom->usbRx_count != 0) {
				pUcom->usbRxFlags |= UCOM_RX_BUF_FULL;
			}
		}
		break;

	case USB_EVT_OUT_NAK:
		/* queue free buffer for RX */
		if ((pUcom->usbRxFlags & (UCOM_RX_BUF_FULL | UCOM_RX_BUF_QUEUED)) == 0) {
			pUcom->usbRx_count = USBD_API->hw->ReadReqEP(hUsb, USB_CDC_OUT_EP, pUcom->usbRx_buff, UCOM_RX_BUF_SZ);
			if(pUcom->usbRx_count){
				if(1 == pUcom->usbRx_count){
					RingBuffer_Insert(&usb_rxrb, pUcom->usbRx_buff);
				}else
				{
					RingBuffer_InsertMult(&usb_rxrb, pUcom->usbRx_buff, pUcom->usbRx_count);
				}
			}
			pUcom->usbRxFlags |= UCOM_RX_BUF_QUEUED;
		}
		break;

	default:
		break;
	}

	return LPC_OK;
}

/* Set line coding call back routine */
static ErrorCode_t UCOM_SetLineCode(USBD_HANDLE_T hCDC, CDC_LINE_CODING *line_coding)
{
	uint32_t config_data = 0;
	UCOM_DATA_T *pUcom = &g_uCOM;

	pUcom->usbTxFlags = UCOM_TX_CONNECTED;

	switch (line_coding->bDataBits) {
	case 5:
		config_data |= UART_LCR_WLEN5;
		break;

	case 6:
		config_data |= UART_LCR_WLEN6;
		break;

	case 7:
		config_data |= UART_LCR_WLEN7;
		break;

	case 8:
	default:
		config_data |= UART_LCR_WLEN8;
		break;
	}

	switch (line_coding->bCharFormat) {
	case 1:	/* 1.5 Stop Bits */
		/* In the UART hardware 1.5 stop bits is only supported when using 5
		 * data bits. If data bits is set to 5 and stop bits is set to 2 then
		 * 1.5 stop bits is assumed. Because of this 2 stop bits is not support
		 * when using 5 data bits.
		 */
		if (line_coding->bDataBits == 5) {
			config_data |= UART_LCR_SBS_2BIT;
		}
		else {
			return ERR_USBD_UNHANDLED;
		}
		break;

	case 2:	/* 2 Stop Bits */
		/* In the UART hardware if data bits is set to 5 and stop bits is set to 2 then
		 * 1.5 stop bits is assumed. Because of this 2 stop bits is
		 * not support when using 5 data bits.
		 */
		if (line_coding->bDataBits != 5) {
			config_data |= UART_LCR_SBS_2BIT;
		}
		else {
			return ERR_USBD_UNHANDLED;
		}
		break;

	default:
	case 0:	/* 1 Stop Bit */
		config_data |= UART_LCR_SBS_1BIT;
		break;
	}

	switch (line_coding->bParityType) {
	case 1:
		config_data |= (UART_LCR_PARITY_EN | UART_LCR_PARITY_ODD);
		break;

	case 2:
		config_data |= (UART_LCR_PARITY_EN | UART_LCR_PARITY_EVEN);
		break;

	case 3:
		config_data |= (UART_LCR_PARITY_EN | UART_LCR_PARITY_F_1);
		break;

	case 4:
		config_data |= (UART_LCR_PARITY_EN | UART_LCR_PARITY_F_0);
		break;

	default:
	case 0:
		config_data |= UART_LCR_PARITY_DIS;
		break;
	}

	if (line_coding->dwDTERate < 3125000) {
		Chip_UART_SetBaud(LPC_USART, line_coding->dwDTERate);
	}
	Chip_UART_ConfigData(LPC_USART, config_data);
	AVALON_led_rgb(AVALON_LED_GREEN);

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
ErrorCode_t UCOM_init(USBD_HANDLE_T hUsb, USB_CORE_DESCS_T *pDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_CDC_INIT_PARAM_T cdc_param;
	ErrorCode_t ret = LPC_OK;
	uint32_t ep_indx;
	USB_CDC_CTRL_T *pCDC;

	/* Store USB stack handle for future use. */
	g_uCOM.hUsb = hUsb;
	/* Initi CDC params */
	memset((void *) &cdc_param, 0, sizeof(USBD_CDC_INIT_PARAM_T));
	cdc_param.mem_base = pUsbParam->mem_base;
	cdc_param.mem_size = pUsbParam->mem_size;
	cdc_param.cif_intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, CDC_COMMUNICATION_INTERFACE_CLASS);
	cdc_param.dif_intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, CDC_DATA_INTERFACE_CLASS);
	cdc_param.SetLineCode = UCOM_SetLineCode;

	/* Init CDC interface */
	ret = USBD_API->cdc->init(hUsb, &cdc_param, &g_uCOM.hCdc);

	if (ret == LPC_OK) {
		/* allocate transfer buffers */
		g_uCOM.usbRx_buff = (uint8_t *) cdc_param.mem_base;
		cdc_param.mem_base += UCOM_RX_BUF_SZ;
		cdc_param.mem_size -= UCOM_RX_BUF_SZ;

		/* register endpoint interrupt handler */
		ep_indx = (((USB_CDC_IN_EP & 0x0F) << 1) + 1);
		ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, UCOM_bulk_hdlr, &g_uCOM);

		if (ret == LPC_OK) {
			/* register endpoint interrupt handler */
			ep_indx = ((USB_CDC_OUT_EP & 0x0F) << 1);
			ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, UCOM_bulk_hdlr, &g_uCOM);
			/* Init UART port for bridging */
			UCOM_UartInit();
			/* Set the line coding values as per UART Settings */
			pCDC = (USB_CDC_CTRL_T *) g_uCOM.hCdc;
			pCDC->line_coding.dwDTERate = 115200;
			pCDC->line_coding.bDataBits = 8;
		}

		/* update mem_base and size variables for cascading calls. */
		pUsbParam->mem_base = cdc_param.mem_base;
		pUsbParam->mem_size = cdc_param.mem_size;
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
	UCOM_DATA_T *pUcom = &g_uCOM;
	uint16_t cnt = 0;

	cnt = RingBuffer_PopMult(&usb_rxrb, (uint8_t *) pBuf, buf_len);
	pUcom->usbRxFlags &= ~UCOM_RX_BUF_FULL;

	return cnt;
}

/* Send data to usb */
uint32_t UCOM_Write(uint8_t *pBuf, uint32_t len)
{
	UCOM_DATA_T *pUcom = &g_uCOM;
	uint32_t ret = 0;

	if ((pUcom->usbTxFlags & UCOM_TX_CONNECTED) && ((pUcom->usbTxFlags & UCOM_TX_BUSY) == 0)) {
		pUcom->usbTxFlags |= UCOM_TX_BUSY;
		ret = USBD_API->hw->WriteEP(pUcom->hUsb, USB_CDC_IN_EP, pBuf, len);
	}

	return ret;
}

/* clear UCOM tx ringbuffer */
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
