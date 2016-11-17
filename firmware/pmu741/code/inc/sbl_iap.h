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
	READ_UID=58
};

int iap_readserialid(uint8_t *dna);

#endif /* _SBL_IAP_H */
