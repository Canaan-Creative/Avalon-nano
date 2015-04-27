/*
 * @brief A3222 head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_A3222_H_
#define __AVALON_A3222_H_

#define A3222_WORK_SIZE		(23*4)
#define A3222_REPORT_SIZE	12 /* work_id (8 bytes) + nonce (4bytes) */
#define A3222_REPORT_CNT	48

void Process(uint8_t *work, const uint32_t *work_id);
uint8_t ReportCnt(void);
uint8_t GetReport(uint8_t *report);


#endif /* __AVALON_A3222_H_ */
