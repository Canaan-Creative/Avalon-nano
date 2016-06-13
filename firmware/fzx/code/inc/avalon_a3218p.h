/*
 * @brief A3218P head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_A3218P_H_
#define __AVALON_A3218P_H_

#define A3218P_WORK_SIZE	(23 * 4)
#define A3218P_WORK_CNT		12
#define A3218P_WORK_LEN		23

#define A3218P_REPORT_SIZE	12 /* work_id (8bytes) + nonce (4bytes) */
#define A3218P_REPORT_CNT	12

#define A3218P_DEFAULT_FREQ  0x1e678447  /* 100 Mhz */

#define DEFAULT_PLLSEL		5
#define A3218P_CMD_WORK   0x00000000
#define A3218P_CMD_CFG    0x00000002
#define A3218P_CMD_INFO0  0x00000003
#define A3218P_CMD_INFO1  0x10000003
#define A3218P_CMD_INFO2  0x20000003
#define A3218P_CMD_INFO3  0x30000003
#define A3218P_CMD_INFO4  0x40000003
#define A3218P_CMD_BYPASS 0x00000004
#define A3218P_CMD_LOOPBACK 0x00000005
#define A3218P_CMD_INVALID 0xffffffff
extern uint8_t printf_buf32[200];

void a3218p_reset(void);
void a3218p_hw_init(void);
void a3218p_sw_init(void);
int a3218p_push_work(uint8_t *pkg);
int a3218p_process(void);

int a3218p_get_works_count(void);
int a3218p_get_report_count(void);
int a3218p_get_report(uint8_t *pkg);

void a3218p_set_freq(uint32_t freq[], uint8_t index);
void a3218p_get_freq(uint32_t freq[], uint8_t index);

void a3218p_set_spispeed(uint32_t speed);
uint32_t a3218p_get_spispeed(void);
void a3218p_bypass_test(void);

#endif /* __AVALON_A3218P_H_ */
