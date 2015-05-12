//-----------------------------------------------------------------------------
// Software that is described herein is for illustrative purposes only
// which provides customers with programming information regarding the
// products. This software is supplied "AS IS" without any warranties.
// NXP Semiconductors assumes no responsibility or liability for the
// use of the software, conveys no license or title under any patent,
// copyright, or mask work right to the product. NXP Semiconductors
// reserves the right to make changes in the software without
// notification. NXP Semiconductors also make no representation or
// warranty that such application will be suitable for the specified
// use without further testing or modification.
//-----------------------------------------------------------------------------
#include <string.h>

#include "sbl_iap.h"
#include "sbl_config.h"

#include "board.h"
#include "libfunctions.h"

#define iap_entry ((void (*)(unsigned [],unsigned []))(IAP_ENTRY_LOCATION))

unsigned g_param_table[5];
unsigned g_result_table[5];
unsigned g_cclk;
char g_flash_buf[FLASH_BUF_SIZE];
unsigned *g_flash_address;
unsigned g_byte_ctr;

static void write_data(unsigned cclk,unsigned flash_address,unsigned * flash_data_buf, unsigned count)
{
	g_param_table[0] = COPY_RAM_TO_FLASH;
	g_param_table[1] = flash_address;
	g_param_table[2] = (unsigned)flash_data_buf;
	g_param_table[3] = count;
	g_param_table[4] = cclk;
	iap_entry(g_param_table, g_result_table);
}

static void erase_sector_usb(unsigned start_sector, unsigned end_sector, unsigned cclk)
{
	g_param_table[0] = ERASE_SECTOR;
	g_param_table[1] = start_sector;
	g_param_table[2] = end_sector;
	g_param_table[3] = cclk;
	iap_entry(g_param_table, g_result_table);
}

static void prepare_sector_usb(unsigned start_sector,unsigned end_sector,unsigned cclk)
{
	g_param_table[0] = PREPARE_SECTOR_FOR_WRITE;
	g_param_table[1] = start_sector;
	g_param_table[2] = end_sector;
	g_param_table[3] = cclk;
	iap_entry(g_param_table, g_result_table);
}

void init_usb_iap(void)
{
	g_cclk = CCLK;
	g_byte_ctr = 0;
	g_flash_address = (unsigned *)UPDATE_REQD;
}

/*
 * UM10462.pdf 20.14.9
 * serial id is 128-bit, we only get the first two words.
 */
int iap_readserialid(char *dna)
{
	uint32_t param_table[5];
	uint32_t result_table[5];

	param_table[0] = IAP_CMD_READUID;
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

static void find_erase_prepare_sector(unsigned cclk, unsigned flash_address)
{
	unsigned i;
	unsigned end_sector;

	end_sector = MAX_USER_SECTOR;
	for (i = 0; i <= end_sector; i++) {
		if (flash_address < (SECTOR_0_START_ADDR + ((i + 1) * SECTOR_SIZE))) {
			if (flash_address == SECTOR_0_START_ADDR + (SECTOR_SIZE * i)) {
				prepare_sector_usb(i, i, cclk);
				erase_sector_usb(i, i, cclk);
			}
			prepare_sector_usb(i, i, cclk);
			break;
		}
	}
}

unsigned write_flash(unsigned * dst, char * src, unsigned no_of_bytes)
{
	unsigned enabled_irqs;

	enabled_irqs = NVIC->ISER[0];

	if (g_flash_address == (unsigned *)UPDATE_REQD) {
		/* Store flash start address */
		g_flash_address = (unsigned *)dst;
	}
	memcpy(&g_flash_buf[g_byte_ctr], src, no_of_bytes);
	g_byte_ctr = g_byte_ctr + no_of_bytes;

	if (g_byte_ctr == FLASH_BUF_SIZE) {
		/* We have accumulated enough bytes to trigger a flash write */
		NVIC->ICER[0] = enabled_irqs;
		find_erase_prepare_sector(g_cclk, (unsigned)g_flash_address);
		if(g_result_table[0] != CMD_SUCCESS) {
			while(1); /* No way to recover. Just let Windows report a write failure */
		}
		write_data(g_cclk,(unsigned)g_flash_address,(unsigned *)g_flash_buf,FLASH_BUF_SIZE);
		if(g_result_table[0] != CMD_SUCCESS) {
			while(1); /* No way to recover. Just let Windows report a write failure */
		}
		/* Reset byte counter and flash address */
		g_byte_ctr = 0;
		g_flash_address = (unsigned *)UPDATE_REQD;
	}
	NVIC->ISER[0] = enabled_irqs;
	return (CMD_SUCCESS);
}
