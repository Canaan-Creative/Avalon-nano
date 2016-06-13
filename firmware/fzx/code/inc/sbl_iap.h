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
#ifndef  _SBL_IAP_H
#define  _SBL_IAP_H

enum iap_cmd_code {
	PREPARE_SECTOR_FOR_WRITE=50,
	COPY_RAM_TO_FLASH=51,
	ERASE_SECTOR=52,
	BLANK_CHECK_SECTOR=53,
	READ_PART_ID=54,
	READ_BOOT_VER=55,
	COMPARE=56,
	REINVOKE_ISP=57,
	READ_UID=58
};

unsigned int write_flash(unsigned int dst, unsigned char *src, unsigned int no_of_bytes);
int iap_readserialid(char *dna);
void reinvokeisp(void);

#endif /* _SBL_IAP_H */
