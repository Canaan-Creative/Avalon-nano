/*
 * @brief Avalon MM protocol head file
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

/* Avalon nano protocol package type, 40 bytes total
 * HEADER(2) TYPE OPT IDX CNT DATA(32) CRC(2)
 * */
#define AVAU_MM_VER_LEN	15

#define AVAU_H1 'C'
#define AVAU_H2 'N'

#define AVAU_P_COUNT    40
#define AVAU_P_DATA_LEN 32

#define AVAU_P_DETECT   0x10

#define AVAU_P_SET_VOLT 0x22
#define AVAU_P_SET_FREQ 0x23
#define AVAU_P_WORK     0x24

#define AVAU_P_POLLING	0x30
#define AVAU_P_REQUIRE	0x31
#define AVAU_P_TEST	0x32

#define AVAU_P_ACKDETECT	0x40
#define AVAU_P_STATUS		0x41
#define AVAU_P_NONCE		0x42
#define AVAU_P_TEST_RET		0x43

#define AVAU_P_DATAOFFSET	6
#define AVAU_P_WORKLEN		64

struct avalon_pkg {
    uint8_t head[2];
    uint8_t type;
    uint8_t opt;
    uint8_t idx;
    uint8_t cnt;
    uint8_t data[AVAU_P_DATA_LEN];
    uint8_t crc[2];
};

#endif	/* _PROTOCOL_H_ */


