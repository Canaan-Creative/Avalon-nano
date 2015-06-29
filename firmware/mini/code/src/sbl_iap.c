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
#include "sbl_iap.h"
#include "libfunctions.h"

#define FLASH_BUF_SIZE	512
#define SECTOR_0_START_ADDR	0
#define SECTOR_SIZE	4096
#define MAX_USER_SECTOR	8
#define CCLK	48000	/* 48,000 KHz for IAP call */

unsigned char g_flash_buf[FLASH_BUF_SIZE];

static int write_data(unsigned int flash_address, unsigned char *data_addr, unsigned int count)
{
	unsigned int param_table[5];
	unsigned int result_table[5];

	param_table[0] = COPY_RAM_TO_FLASH;
	param_table[1] = flash_address;
	param_table[2] = (unsigned int)data_addr;
	param_table[3] = count;
	param_table[4] = CCLK;
	iap_entry(param_table, result_table);

	return result_table[0];
}

static int erase_sector_usb(unsigned int start_sector, unsigned int end_sector)
{
	unsigned int param_table[5];
	unsigned int result_table[5];

	param_table[0] = ERASE_SECTOR;
	param_table[1] = start_sector;
	param_table[2] = end_sector;
	param_table[3] = CCLK;
	iap_entry(param_table, result_table);

	return result_table[0];
}

static int prepare_sector_usb(unsigned int start_sector, unsigned int end_sector)
{
	unsigned int param_table[5];
	unsigned int result_table[5];

	param_table[0] = PREPARE_SECTOR_FOR_WRITE;
	param_table[1] = start_sector;
	param_table[2] = end_sector;
	iap_entry(param_table, result_table);

	return result_table[0];
}

/*
 * UM10462.pdf 20.14.9
 * serial id is 128-bit, we only get the first two words.
 */
int iap_readserialid(char *dna)
{
	unsigned int param_table[5];
	unsigned int result_table[5];

	param_table[0] = READ_UID;
	iap_entry(param_table, result_table);
	if (!result_table[0]) {
		result_table[1] = be32toh(result_table[1]);
		memcpy(dna, &result_table[1], 4);
		result_table[2] = be32toh(result_table[2]);
		memcpy(dna + 4, &result_table[2], 4);
		return 0;
	}

	return 1;
}

void reinvokeisp(void)
{
	unsigned int param_table[5];
	unsigned int result_table[5];

	__disable_irq();
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT32B1);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_GPIO);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_Clock_SetSysClockDiv(1);

	/* Set stack pointer to ROM value (reset default).
	 This must be the last piece of code executed before calling ISP,
	 because most C expressions and function returns will fail after
	 the stack pointer is changed.
	 UM10462 20.15 Debug notes
	 */
	__set_MSP(*((uint32_t *) 0x1FFF0000));
	param_table[0] = REINVOKE_ISP;
	/* It will not return */
	iap_entry(param_table, result_table);
}

static int find_erase_prepare_sector(unsigned int flash_address)
{
	unsigned int i;

	for (i = 0; i < MAX_USER_SECTOR; i++) {
		if (flash_address < (SECTOR_0_START_ADDR + ((i + 1) * SECTOR_SIZE))) {
			if (flash_address == SECTOR_0_START_ADDR + (SECTOR_SIZE * i)) {
				if (prepare_sector_usb(i, i))
					return 1;
				if (erase_sector_usb(i, i))
					return 1;
			}

			if (prepare_sector_usb(i, i))
				return 1;
			break;
		}
	}

	return 0;
}

unsigned int write_flash(unsigned int dst, unsigned char *src, unsigned int no_of_bytes)
{
	static unsigned int byte_cnt = 0;
	unsigned int enabled_irqs;
	int ret = 0;

	enabled_irqs = NVIC->ISER[0];

	memcpy(&g_flash_buf[byte_cnt], src, no_of_bytes);
	byte_cnt += no_of_bytes;

	if (byte_cnt == FLASH_BUF_SIZE) {
		NVIC->ICER[0] = enabled_irqs;
		byte_cnt = 0;
		if (find_erase_prepare_sector(dst - FLASH_BUF_SIZE + 1))
			ret = 1;
		if (write_data(dst - FLASH_BUF_SIZE + 1, g_flash_buf, FLASH_BUF_SIZE))
			ret = 1;
	}
	NVIC->ISER[0] = enabled_irqs;

	return ret;
}

