/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include "board.h"
#include "avalon_wdt.h"

/**
 * @brief	Watchdog Timer Interrupt Handler
 * @return	Nothing
 * @note	Handles watchdog timer warning and timeout events
 */
void WDT_IRQHandler(void)
{
	uint32_t wdtStatus = Chip_WWDT_GetStatus(LPC_WWDT);

#if defined(CHIP_LPC11CXX)
	if (wdtStatus & WWDT_WDMOD_WDTOF) {
		Board_LED_Toggle(0);
		while(Chip_WWDT_GetStatus(LPC_WWDT) & WWDT_WDMOD_WDTOF) {
			Chip_WWDT_ClearStatusFlag(LPC_WWDT, WWDT_WDMOD_WDTOF);
		}
		Chip_WWDT_Start(LPC_WWDT);	/* Needs restart */
	}
#else
	/* The chip will reset before this happens, but if the WDT doesn't
	   have WWDT_WDMOD_WDRESET enabled, this will hit once */
	if (wdtStatus & WWDT_WDMOD_WDTOF) {
		/* A watchdog feed didn't occur prior to window timeout */
		Chip_WWDT_ClearStatusFlag(LPC_WWDT, WWDT_WDMOD_WDTOF);
	}
#endif
}

void wdt_init(uint8_t second)
{
	uint32_t wdtFreq;

	/* Initialize WWDT (also enables WWDT clock) */
	Chip_WWDT_Init(LPC_WWDT);

	/* Prior to initializing the watchdog driver, the clocking for the
	   watchdog must be enabled. This example uses the watchdog oscillator
	   set at a 50KHz (1Mhz / 20) clock rate. */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_WDTOSC_PD);
	Chip_Clock_SetWDTOSC(WDTLFO_OSC_1_05, 20);

	/* The WDT divides the input frequency into it by 4 */
	wdtFreq = Chip_Clock_GetWDTOSCRate() / 4;

	/* LPC1102/4, LPC11XXLV, and LPC11CXX devices select the watchdog
	   clock source from the SYSCLK block, while LPC11AXX, LPC11EXX, and
	   LPC11UXX devices select the clock as part of the watchdog block. */
	/* Select watchdog oscillator for WDT clock source */
#if defined(CHIP_LPC110X) || defined(CHIP_LPC11XXLV) || defined(CHIP_LPC11CXX) || defined(CHIP_LPC11EXX) || defined(CHIP_LPC1125)
	Chip_Clock_SetWDTClockSource(SYSCTL_WDTCLKSRC_WDTOSC, 1);
#else
	Chip_WWDT_SelClockSource(LPC_WWDT, WWDT_CLKSRC_WATCHDOG_WDOSC);
#endif

	Chip_WWDT_SetTimeOut(LPC_WWDT, wdtFreq * second);

#if !defined(CHIP_LPC11CXX)
	/* Configure WWDT to reset on timeout */
	Chip_WWDT_SetOption(LPC_WWDT, WWDT_WDMOD_WDRESET);
#endif

	/* Clear watchdog warning and timeout interrupts */
#if !defined(CHIP_LPC11CXX)
	Chip_WWDT_ClearStatusFlag(LPC_WWDT, WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT);
#else
	Chip_WWDT_ClearStatusFlag(LPC_WWDT, WWDT_WDMOD_WDTOF);
#endif

	/* Clear and enable watchdog interrupt */
	NVIC_ClearPendingIRQ(WDT_IRQn);
	NVIC_EnableIRQ(WDT_IRQn);
}

void wdt_enable(void)
{
	/* Start watchdog */
	Chip_WWDT_Start(LPC_WWDT);
}

void wdt_feed(void)
{
	Chip_WWDT_Feed(LPC_WWDT);
}

