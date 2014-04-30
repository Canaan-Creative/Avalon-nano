/*
===============================================================================
 Name        : main.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : main definition
===============================================================================
*/
#include "board.h"
#include <cr_section_macros.h>
#include <NXP/crp.h>
#include "avalon_api.h"

#define AVALON_USB_TEST			(1)
#define AVALON_PWM_TEST			(1)
#define AVALON_LED_TEST			(1)
#define AVALON_A3233_TEST		(1)

__CRP unsigned int CRP_WORD = CRP_NO_ISP;

int main(void) {
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
	/* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);

    AVALON_USB_Init();

    /* wait a little time for open usb cdc */
    AVALON_Delay(5000000);

    // Force the counter to be placed into memory
    volatile static int i = 0 ;
    // Enter an infinite loop, just incrementing a counter
    while(1) {
#if AVALON_USB_TEST
    	AVALON_USB_Test();
#endif

#if AVALON_LED_TEST
    	AVALON_USB_PutSTR("AVALON_LED_Test Start\n");
    	AVALON_LED_Test();
    	AVALON_USB_PutSTR("AVALON_LED_Test End\n");
#endif

#if AVALON_PWM_TEST
    	AVALON_USB_PutSTR("AVALON_Pwm_Test Start\n");
    	AVALON_Pwm_Test();
    	AVALON_USB_PutSTR("AVALON_Pwm_Test End\n");
#endif

#if AVALON_A3233_TEST
    	AVALON_USB_PutSTR("AVALON_A3233_Test Start\n");
    	AVALON_A3233_Test();
    	AVALON_USB_PutSTR("AVALON_A3233_Test End\n");
#endif
        i++ ;
    }
    return 0 ;
}
