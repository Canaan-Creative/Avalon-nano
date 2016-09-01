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
#include <stdint.h>
#include "board.h"
#include "sbl_iap.h"
#include "libfunctions.h"

/*
 * UM10462.pdf 20.14.9
 * serial id is 128-bit, we only get the first two words.
 */
int iap_readserialid(uint8_t *dna)
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

