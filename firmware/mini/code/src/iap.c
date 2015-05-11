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

#include "board.h"
#include "libfunctions.h"

#define IAP_CMD_READUID 58

#define iap_entry ((void (*)(unsigned [],unsigned []))(IAP_ENTRY_LOCATION))

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

