/*
 * @brief Vitual communication port example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "board.h"
#include <stdio.h>
#include <string.h>
#include "app_usbd_cfg.h"
#include "cdc_vcom.h"
#include "sha2.h"
#include <NXP/crp.h>

#define TMP_MAX 50

__CRP unsigned int CRP_WORD = CRP_NO_ISP;
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
/* Transmit and receive ring buffers */
STATIC RINGBUFF_T txring, rxring;

/* Transmit and receive ring buffer sizes */
#define UART_SRB_SIZE 128	/* Send */
#define UART_RRB_SIZE 32	/* Receive */

#define UART_DELAY

/* Transmit and receive buffers */
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];

const char inst1[] = "LPC11xx UART example using ring buffers\r\n";
const char inst2[] = "Press a key to echo it back or ESC to quit\r\n";
/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

static USBD_HANDLE_T g_hUsb;
static uint8_t g_rxBuff[256];

extern const  USBD_HW_API_T hw_api;
extern const  USBD_CORE_API_T core_api;
extern const  USBD_CDC_API_T cdc_api;
/* Since this example only uses CDC class link functions for that clas only */
static const  USBD_API_T g_usbApi = {
	&hw_api,
	&core_api,
	0,
	0,
	0,
	&cdc_api,
	0,
	0x02221101,
};

const  USBD_API_T *g_pUsbApi = &g_usbApi;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initialize pin and clocks for USB0/USB1 port */
static void usb_pin_clk_init(void)
{
	/* enable USB main clock */
	Chip_Clock_SetUSBClockSource(SYSCTL_USBCLKSRC_PLLOUT, 1);
	/* Enable AHB clock to the USB block and USB RAM. */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USB);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USBRAM);
	/* power UP USB Phy */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPAD_PD);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from USB0
 * @return	Nothing
 */
void USB_IRQHandler(void)
{
	USBD_API->hw->ISR(g_hUsb);
}

/* Find the address of interface descriptor for given class type. */
USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass)
{
	USB_COMMON_DESCRIPTOR *pD;
	USB_INTERFACE_DESCRIPTOR *pIntfDesc = 0;
	uint32_t next_desc_adr;

	pD = (USB_COMMON_DESCRIPTOR *) pDesc;
	next_desc_adr = (uint32_t) pDesc;

	while (pD->bLength) {
		/* is it interface descriptor */
		if (pD->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) {

			pIntfDesc = (USB_INTERFACE_DESCRIPTOR *) pD;
			/* did we find the right interface descriptor */
			if (pIntfDesc->bInterfaceClass == intfClass) {
				break;
			}
		}
		pIntfDesc = 0;
		next_desc_adr = (uint32_t) pD + pD->bLength;
		pD = (USB_COMMON_DESCRIPTOR *) next_desc_adr;
	}

	return pIntfDesc;
}

static void Init_CLKOUT_PinMux(void)
{
#if (defined(BOARD_NXP_XPRESSO_11U14) || defined(BOARD_NGX_BLUEBOARD_11U24))
	/* LPC11U14 Xpresso board has CLKOUT on pin PIO0_1 on J6-38 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
#elif defined(BOARD_NXP_XPRESSO_11C24)
	/* LPC11C24 Xpresso board has CLKOUT on pin PIO0_1 on J6-38 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO0_1, (IOCON_FUNC1 | IOCON_MODE_INACT));
#elif defined(BOARD_MCORE48_1125)
	/* LPC1125 MCore48 board has CLKOUT on pin PIO0_1 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO0_1, (IOCON_FUNC1 | IOCON_MODE_INACT));
#else
	#error "Pin MUX for CLKOUT not configured"
#endif
}

static void CLKOUT_Cfg(bool On){
	if(On == true)
		Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);
	else
		Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 0);
}

unsigned int gen_test_a3233(unsigned int *buf, unsigned int cpm_cfg){

   buf[0] = 0x11111111;
   buf[1] = cpm_cfg;
   buf[2] = 0x00000000;
   buf[3] = 0x4ac1d001;
   buf[4] = 0x89517050;
   buf[5] = 0x087e051a;
   buf[6] = 0x06b168ae;
   buf[7] = 0x62a5f25c;
   buf[8] = 0x00639107;
   buf[9] = 0x13cdfd7b;
   buf[10] = 0xfa77fe7d;
   buf[11] = 0x9cb18a17;
   buf[12] = 0x65c90d1e;
   buf[13] = 0x8f41371d;
   buf[14] = 0x974bf4bb;
   buf[15] = 0x7145fd6d;
   buf[16] = 0xc44192c0;
   buf[17] = 0x12146495;
   buf[18] = 0xd8f8ef67;
   buf[19] = 0xa2cb45c1;
   buf[20] = 0x1bee2ba0;
   buf[21] = 0xaaaaaaaa;
   //Chip_UART_Send(LPC_USART, (unsigned char *)buf, 22*4);
   return buf[20] + 0x6000;
}

static void Init_UART_PinMux(void)
{
#if (defined(BOARD_NXP_XPRESSO_11U14) || defined(BOARD_NGX_BLUEBOARD_11U24))
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_MODE_PULLUP);	/* PIO0_18 used for RXD */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 19, IOCON_FUNC1 | IOCON_MODE_INACT);	/* PIO0_19 used for TXD */
#elif (defined(BOARD_NXP_XPRESSO_11C24) || defined(BOARD_MCORE48_1125))
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_6, (IOCON_FUNC1 | IOCON_MODE_INACT));/* RXD */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_7, (IOCON_FUNC1 | IOCON_MODE_INACT));/* TXD */
#else
#error "No Pin muxing defined for UART operation"
#endif
}

void UART_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	Chip_UART_IRQRBHandler(LPC_USART, &rxring, &txring);
}

static void POWER_Enable(bool On){
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 11, On);//VCore Enable
}

static void Init_POWER(){
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);//VID0
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);//VID1
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 11);//VCore Enable
	*(unsigned int *) 0x4004402c = 0x81;

	POWER_Enable(false);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, true);//VID0
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, true);//VID1
}

#define VCORE_0P9   0x0
#define VCORE_0P8   0x1
#define VCORE_0P725 0x2
#define VCORE_0P675 0x3

static void POWER_Cfg(unsigned char VID){
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, (bool)(VID&1));//VID0
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, (bool)(VID>>1));//VID1
}

static void Init_Rstn(){
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
}

static void Rstn_A3233(){
	delay(2000);
	delay(2000);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
	delay(2000);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, false);
	delay(2000);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
	delay(2000);
}

/*i2c*/
#define I2C_NOP 4 //even only
#define I2C_ADDR_W 0x92 //write:0x92, read:0x93
#define I2C_ADDR_R 0x93 //write:0x92, read:0x93
static void Init_I2c(){
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
}

static void I2c_Start(){
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	delay(I2C_NOP);
}

static void I2c_Stop(){
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	delay(I2C_NOP);
}

static void I2c_w_byte(unsigned char data){
	unsigned int i;
	unsigned char data_buf = data;
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	delay(I2C_NOP);

	for(i=0; i<8; i++){
		if((data_buf&0x80) == 0x80)
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
		delay(I2C_NOP);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
		delay(I2C_NOP);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
		data_buf = data_buf << 1;
		delay(I2C_NOP);
	}
	//master wait ACK
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
	delay(I2C_NOP);
}

unsigned char I2c_r_byte(){
	unsigned int i;
	unsigned char data_buf = 0;

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 5);

	for(i=0; i<8; i++){
		delay(I2C_NOP);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
		delay(I2C_NOP/2);
		data_buf = data_buf << 1;
		data_buf = data_buf | (Chip_GPIO_ReadPortBit(LPC_GPIO, 0, 5)&0x1);
		delay(I2C_NOP/2);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
		delay(I2C_NOP);
	}

	//master sent ACK
	delay(I2C_NOP);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	delay(I2C_NOP);

	return data_buf;
}

unsigned int tmp102_rd(){
	unsigned int tmp = 0;
	I2c_Start();
	I2c_w_byte(I2C_ADDR_W);
	I2c_w_byte(0x0);//temperature register
	I2c_Stop();
	I2c_Start();
	I2c_w_byte(I2C_ADDR_R);
	tmp = I2c_r_byte()&0xff;
	tmp = tmp << 8;
	tmp = (tmp&0xffffff00) | I2c_r_byte();
	I2c_Stop();
	tmp = (((tmp >> 4)&0xfff)/4)*0.25;
	return tmp;
}

/*ACD*/
static void Init_ADC_PinMux(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, FUNC1);
}

static void ADC_Rd(uint8_t channel, uint16_t *data){
	unsigned char rxc;
	Chip_ADC_EnableChannel(LPC_ADC, channel, ENABLE);
	/* Start A/D conversion */
	Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, channel, ADC_DR_DONE_STAT) != SET) {}
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, channel, data);
	Chip_ADC_EnableChannel(LPC_ADC, channel, DISABLE);
}

static void ADC_Guard(){
	uint16_t dataADC;
	float vol = 0;

	ADC_Rd(ADC_CH1, &dataADC);
	vol = (dataADC/1024) * 3.3;//5/2 = 2.5V

	ADC_Rd(ADC_CH3, &dataADC);
	vol = (dataADC/1024) * 3.3;//1.8V

	ADC_Rd(ADC_CH5, &dataADC);
	vol = (dataADC/1024) * 3.3;//VCore

	ADC_Rd(ADC_CH7, &dataADC);
	vol = (dataADC/1024) * 3.3;//0.9V
}

#ifdef HIGH_BAND
#define FREF_MIN 25
#define FREF_MAX 50
#define FVCO_MIN 1500
#define FVCO_MAX 3000
#define FOUT_MIN 187.5
#define FOUT_MAX 3000
#define WORD0_BASE 0x80000007
#else
#define FREF_MIN 10
#define FREF_MAX 50
#define FVCO_MIN 800
#define FVCO_MAX 1600
#define FOUT_MIN 100
#define FOUT_MAX 1600
#define WORD0_BASE 0x7
#endif

unsigned int Gen_A3233_Pll_Cfg(unsigned int freq){
	unsigned int NOx[4] , i=0;
	unsigned int NO =0;//1 2 4 8
	unsigned int Fin = 25 ;
	unsigned int NR =0;
	unsigned int Fvco =0;
	unsigned int Fout = 200 ;
	unsigned int NF =0;
	unsigned int Fref =0;
	unsigned int OD ;
	unsigned int tmp ;
	NOx[0] = 1 ;
	NOx[1] = 2 ;
	NOx[2] = 4 ;
	NOx[3] = 8 ;

	for(Fout = freq ; Fout <= 1300 ; Fout=Fout+1)
	for(i=0;i<4;i++){
		NO = NOx[i] ;
		for(NR=1;NR<32;NR++)
			for(NF=1;NF<128;NF++){
				Fref = Fin/NR ;
				Fvco = Fout*NO ;
				if(
				((Fout) == ((Fin*NF/(NR*NO))) ) &&
				(FREF_MIN<=Fref&&Fref<=FREF_MAX) &&
				(FVCO_MIN<=Fvco&&Fvco<=FVCO_MAX) &&
				(FOUT_MIN<=Fout      <=FOUT_MAX)
				){
					if(NO == 1) OD = 0 ;
					if(NO == 2) OD = 1 ;
					if(NO == 4) OD = 2 ;
					if(NO == 8) OD = 3 ;

					tmp =   WORD0_BASE     |
						((NR-1)&0x1f)<<16  |
						((NF/2-1)&0x7f)<<21|
						(OD<<28) ;

					return tmp;
				}
		}
	}
}

#define LED_GREEN 	0
#define LED_RED		1
#define LED_BLUE	2
#define LED_OFF		3
void led_rgb(unsigned int rgb)
{
	if (rgb == LED_GREEN) {
		Board_LED_Set(0, false);//green
		Board_LED_Set(1, true);//red
		Board_LED_Set(2, true);//blue
	} else if (rgb == LED_RED) {
		Board_LED_Set(0, true);//green
		Board_LED_Set(1, false);//red
		Board_LED_Set(2, true);//blue
	} else if (rgb == LED_BLUE) {
		Board_LED_Set(0, true);//green
		Board_LED_Set(1, true);//red
		Board_LED_Set(2, false);//blue
	} else {
		Board_LED_Set(0, true);//green
		Board_LED_Set(1, true);//red
		Board_LED_Set(2, true);//blue
	}
}

void Init_Pwm()
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT16B0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 15, IOCON_FUNC2 | IOCON_MODE_INACT);//red
	*(unsigned int *)0x4000c004 = 0x1;//enable tcr, ct16b0
	*(unsigned int *)0x4000c008 = 0xffff;//timer counter
	*(unsigned int *)0x4000c03c = 0x1 | (0x3<<8);//External Match
	*(unsigned int *)0x4000c074 = 0x4;//pwm
	*(unsigned int *)0x4000c020 = 0xffff/4;
}

void Init_Gpio(){
	Chip_GPIO_Init(LPC_GPIO);
}

static void Init_Led(void)
{
	/* Set the PIO_7 as output */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 17);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 15);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 19);
}

void Init_Counter(){
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT32B0);
}

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
unsigned char rx_buf[22*4];
static ADC_CLOCK_SETUP_T ADCSetup;
#define IN_BUF_LEN 64

int main(void)
{
	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;
	uint32_t prompt = 0, rdCnt = 0;

	unsigned char work_buf[22*4];
	unsigned int pll_cfg0 = 0x1;
	unsigned char nonce_buf[128];
	unsigned int nonce_cnt;
	unsigned int nonce_i;
	unsigned int nonce_wd;

	SystemCoreClockUpdate();
	Init_Gpio();
	Init_Led();
	Init_Rstn();//low active
	Init_CLKOUT_PinMux();
	Init_UART_PinMux();
	Init_POWER();
	Init_I2c();
	Init_ADC_PinMux();
	Chip_ADC_Init(LPC_ADC, &ADCSetup);

	POWER_Cfg(VCORE_0P675);
	CLKOUT_Cfg(true);
	POWER_Enable(false);
	Rstn_A3233();

	led_rgb(LED_BLUE);

	/* enable clocks and pinmux */
	usb_pin_clk_init();

	/* Setup UART for 115.2K8N1 */
	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaud(LPC_USART, 111111);//115200);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Enable receive data and line status interrupt */
	//Chip_UART_IntEnable(LPC_USART, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(UART0_IRQn, 1);
	NVIC_EnableIRQ(UART0_IRQn);

	/* initilize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;
	usb_param.max_num_ep = 3;
	usb_param.mem_base = USB_STACK_MEM_BASE;
	usb_param.mem_size = USB_STACK_MEM_SIZE;

	/* Set the USB descriptors */
	desc.device_desc = (uint8_t *) &USB_DeviceDescriptor[0];
	desc.string_desc = (uint8_t *) &USB_StringDescriptor[0];
	/* Note, to pass USBCV test full-speed only devices should have both
	   descriptor arrays point to same location and device_qualifier set to 0.
	 */
	desc.high_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.full_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.device_qualifier = 0;

	/* USB Initialization */
	ret = USBD_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK) {

		/* Init VCOM interface */
		ret = vcom_init(g_hUsb, &desc, &usb_param);
		if (ret == LPC_OK) {
			/*  enable USB interrrupts */
			NVIC_EnableIRQ(USB0_IRQn);
			/* now connect */
			USBD_API->hw->Connect(g_hUsb, 1);
		}

	}

	DEBUGSTR("USB CDC class based virtual Comm port example!\r\n");

	pll_cfg0 = Gen_A3233_Pll_Cfg(300);

	POWER_Enable(true);
	Rstn_A3233();

	while (1) {
		/* Check if host has connected and opened the VCOM port */
		if ((vcom_connected() != 0) && (prompt == 0)) {
			prompt = 1;
		}

		if (prompt) {
			rdCnt = 0;
			while (1) {
				rdCnt += vcom_bread(&g_rxBuff[rdCnt], IN_BUF_LEN - rdCnt);
				if (rdCnt >= IN_BUF_LEN)
					break;
			}

			data_pkg(&g_rxBuff[0], &work_buf[0]);
			((unsigned int *)work_buf)[1] = pll_cfg0;

			if (rdCnt) {
					/*Buffer to A3233*/
					led_rgb(LED_OFF);

					Chip_UART_Send(LPC_USART, work_buf, 22 * 4);
					Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2 | UART_FCR_RX_RS));

					led_rgb(LED_GREEN);
					nonce_cnt = 0;
					nonce_i = 0;

					while (1) {
						if (((*(unsigned int *)0x40008014) & 0x1) == 0x1) {
							nonce_buf[nonce_cnt] = *(unsigned int *)0x40008000;
							nonce_cnt++;
							if (vcom_rx_cnt())
								break;
						}

						if (vcom_rx_cnt())
							break;

						if (nonce_cnt >= 4) {
							led_rgb(LED_RED);

							nonce_wd = nonce_buf[nonce_i*4+0]   |
									(nonce_buf[nonce_i*4+1]<<8 )|
									(nonce_buf[nonce_i*4+2]<<16)|
									(nonce_buf[nonce_i*4+3]<<24);
							nonce_wd = nonce_wd - 0x1000;
							nonce_buf[nonce_i*4+0] = nonce_wd&0xff;
							nonce_buf[nonce_i*4+1] = (nonce_wd&0xff00    )>>8 ;
							nonce_buf[nonce_i*4+2] = (nonce_wd&0xff0000  )>>16;
							nonce_buf[nonce_i*4+3] = (nonce_wd&0xff000000)>>24;

							vcom_write(&nonce_buf[nonce_i*4], 4);
							nonce_cnt = 0;
							break;
						}
					}
					led_rgb(LED_BLUE);
			}
		}
		/* Sleep until next IRQ happens */
		__WFI();
	}
}
