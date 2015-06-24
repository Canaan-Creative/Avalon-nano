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
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "app_usbd_cfg.h"
#include "board.h"
#include "dfu.h"
#include "sbl_iap.h"

/* DFU boot definitions */
#define DFU_DEST_BASE	0x0000
#define DFU_SEC_SIZE	(USB_DFU_XFER_SIZE / 4)
#define DFU_MAX_BLOCKS	64

static volatile uint8_t dfu_detach_sig = 0;
extern const uint8_t USB_dfuConfigDescriptor[];

static uint32_t dfu_read(uint32_t block_num, uint8_t **pBuff, uint32_t length)
{
	uint32_t src_addr = DFU_DEST_BASE;

	if (length) {
		if (block_num == DFU_MAX_BLOCKS)
			return 0;

		if (block_num > DFU_MAX_BLOCKS)
			return DFU_STATUS_errADDRESS;

		src_addr += (block_num * DFU_SEC_SIZE);
		memcpy((void*)(*pBuff), (void*)src_addr, length);
	}
	return length;
}

static uint8_t dfu_write(uint32_t block_num, uint8_t **pBuff, uint32_t length, uint8_t *bwPollTimeout)
{
	bwPollTimeout[0] = 255;

	if (length != 0) {
		uint32_t dest_addr = DFU_DEST_BASE;

		if (block_num >= DFU_MAX_BLOCKS)
			return DFU_STATUS_errADDRESS;

		dest_addr += (block_num * DFU_SEC_SIZE);
		write_flash(dest_addr, (unsigned char*)&((*pBuff)[0]), length);
	}

	return DFU_STATUS_OK;
}

static void dfu_done(void)
{
	return;
}

static void dfu_detach(USBD_HANDLE_T hUsb)
{
	USB_CORE_CTRL_T* pCtrl = (USB_CORE_CTRL_T*)hUsb;

	pCtrl->full_speed_desc = (uint8_t *) &USB_dfuConfigDescriptor[0];
	pCtrl->high_speed_desc = (uint8_t *) &USB_dfuConfigDescriptor[0];

	dfu_detach_sig = 1;
}

ErrorCode_t dfu_init(USBD_HANDLE_T hUsb, USB_INTERFACE_DESCRIPTOR* pIntfDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_DFU_INIT_PARAM_T dfu_param;
	ErrorCode_t ret = LPC_OK;

	/* DFU paramas */
	memset((void*)&dfu_param, 0, sizeof(USBD_DFU_INIT_PARAM_T));
	dfu_param.mem_base = pUsbParam->mem_base;
	dfu_param.mem_size = pUsbParam->mem_size;
	dfu_param.wTransferSize = DFU_SEC_SIZE;

	if ((pIntfDesc == 0) ||
			(pIntfDesc->bInterfaceClass != USB_DEVICE_CLASS_APP) ||
			(pIntfDesc->bInterfaceSubClass != USB_DFU_SUBCLASS) )
		return ERR_FAILED;

	dfu_param.intf_desc = (uint8_t*)pIntfDesc;
	/* user defined functions */
	dfu_param.DFU_Write = dfu_write;
	dfu_param.DFU_Read = dfu_read;
	dfu_param.DFU_Done = dfu_done;
	dfu_param.DFU_Detach = dfu_detach;

	ret = USBD_API->dfu->init(hUsb, &dfu_param, 0);
	/* update memory variables */
	pUsbParam->mem_base = dfu_param.mem_base;
	pUsbParam->mem_size = dfu_param.mem_size;
	return ret;
}

uint8_t dfu_sig(void)
{
	return dfu_detach_sig;
}

void dfu_proc(void)
{
	dfu_detach_sig = 0;
}

