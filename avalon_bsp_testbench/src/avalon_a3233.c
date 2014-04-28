/*
===============================================================================
 Name        : avalon_a3233.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon a3233 api
===============================================================================
*/
#include <stdint.h>
#include <string.h>
#include "sha2.h"
#include "cdc_uart.h"
#include "avalon_api.h"

#define V_25	0
#define V_18	1
#define V_CORE	2
#define V_09	3
static float AVALON_A3233_ADCGuard(int type)
{
	uint16_t dataADC;
	float vol;
	switch (type) {
	case V_25:
		AVALON_ADC_Rd(ADC_CH1, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	case V_18:
		AVALON_ADC_Rd(ADC_CH3, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	case V_CORE:
		AVALON_ADC_Rd(ADC_CH5, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	case V_09:
		AVALON_ADC_Rd(ADC_CH7, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	}

	return vol;
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

/*
 * @brief	gen pll cfg val (freq 200-400 )
 * @return	pll cfg val
 * */
static unsigned int AVALON_A3233_PllCfgGen(unsigned int freq)
{
	unsigned int NOx[4] , i=0;
	unsigned int NO =0;//1 2 4 8
	unsigned int Fin = 12;
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
				(FOUT_MIN<=Fout&&Fout<=FOUT_MAX)
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

	return 0;
}

static void AVALON_A3233_CLKOUTInit(void)
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

static void AVALON_A3233_CLKOUTCfg(bool On)
{
	if(On == TRUE)
		Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 4);
	else
		Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 0);
}

static void AVALON_A3233_Rstn()
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, TRUE);
}

void AVALON_A3233_PowerEn(Bool On)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 11, On);//VCore Enable
}

static void AVALON_A3233_PowerInit()
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);//VID0
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);//VID1
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 11);//VCore Enable
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 12);//Power Good

	*(unsigned int *) 0x4004402c = 0x81;

	AVALON_A3233_PowerEn(FALSE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, TRUE);//VID0
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, TRUE);//VID1
}

static Bool AVALON_A3233_IsPowerGood()
{
	return Chip_GPIO_ReadPortBit(LPC_GPIO, 0, 12);
}

#define VCORE_0P9   0x0
#define VCORE_0P8   0x1
#define VCORE_0P725 0x2
#define VCORE_0P675 0x3

static void AVALON_A3233_PowerCfg(unsigned char VID){
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, (bool)(VID&1));//VID0
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, (bool)(VID>>1));//VID1
}

#define I2C_ADDR_W 0x92 //write:0x92, read:0x93
#define I2C_ADDR_R 0x93 //write:0x92, read:0x93

static unsigned int AVALON_A3233_TemperRd(){
	unsigned int tmp = 0;
	AVALON_I2c_Start();
	AVALON_I2c_Wbyte(I2C_ADDR_W);
	AVALON_I2c_Wbyte(0x0);//temperature register
	AVALON_I2c_Stop();
	AVALON_I2c_Start();
	AVALON_I2c_Wbyte(I2C_ADDR_R);
	tmp = AVALON_I2c_Rbyte()&0xff;
	tmp = tmp << 8;
	tmp = (tmp&0xffffff00) | AVALON_I2c_Rbyte();
	AVALON_I2c_Stop();
	tmp = (((tmp >> 4)&0xfff)/4)*0.25;
	return tmp;
}


void AVALON_A3233_Init(void)
{
	static Bool bA3233Init = FALSE;
	ADC_CLOCK_SETUP_T ADCSetup;

	if(bA3233Init)
		return;

	Chip_GPIO_Init(LPC_GPIO);
	AVALON_A3233_Rstn();
	AVALON_A3233_CLKOUTInit();
	AVALON_A3233_PowerInit();

	AVALON_ADC_Init();
	Chip_ADC_Init(LPC_ADC, &ADCSetup);

	AVALON_I2c_Init();

	AVALON_A3233_PowerCfg(VCORE_0P675);
	AVALON_A3233_CLKOUTCfg(TRUE);
	AVALON_A3233_PowerEn(FALSE);
	bA3233Init = TRUE;
}

/* convert radix */
char *myitoa( int num, char*str, int radix)
{
	char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	unsigned unum;
	int i = 0, j, k;

	if (radix == 10 && num < 0)
	{
		unum = (unsigned) -num;
		str[i++] = '-';
	}
	else
		unum = (unsigned) num;

	do {
		str[i++] = index[unum % (unsigned) radix];
		unum /= radix;
	} while (unum);
	str[i] = '\0';

	if (str[0] == '-')
		k = 1;
	else
		k = 0;
	char temp;
	for (j = k; j <= (i - 1) / 2; j++) {
		temp = str[j];
		str[j] = str[i - 1 + k - j];
		str[i - 1 + k - j] = temp;
	}
	return str;
}

static unsigned char golden_ob[] = "\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";

#define A3233_TASK_LEN 88
#define A3233_NONCE_LEN	4
#define ICA_TASK_LEN 64

/* infinite loop */
void AVALON_A3233_Test(void)
{
	char 		dbgbuf[100];
	char		strbuf[20];
	uint8_t 	icarus_buf[ICA_TASK_LEN];
	unsigned char 	work_buf[A3233_TASK_LEN];
	unsigned char	nonce_buf[A3233_NONCE_LEN];
	uint32_t 		nonce_value = 0;
	Bool			findnonce = FALSE;

	/* enable usb for debug purpose */
    AVALON_USB_Init();

    AVALON_A3233_Init();

    strcpy(dbgbuf, "after init powergood = ");
	myitoa(AVALON_A3233_IsPowerGood(), strbuf, 10);
	strcat(dbgbuf, strbuf);
	strcat(dbgbuf,"\n");
	AVALON_USB_PutSTR(dbgbuf);

    memcpy(icarus_buf,golden_ob,ICA_TASK_LEN);
	data_convert(icarus_buf);
	data_pkg(icarus_buf, work_buf);

	/* enable a3233 */
	AVALON_A3233_PowerEn(TRUE);
	AVALON_A3233_Rstn();

    strcpy(dbgbuf, "after poweron powergood = ");
	myitoa(AVALON_A3233_IsPowerGood(), strbuf, 10);
	strcat(dbgbuf, strbuf);
	strcat(dbgbuf,"\n");
	AVALON_USB_PutSTR(dbgbuf);

	((unsigned int*)work_buf)[1] = AVALON_A3233_PllCfgGen(400);

	/* for valid nonce */
	work_buf[81] = 0x1;
	work_buf[82] = 0x73;
	work_buf[83] = 0xa2;
	UART_Write(work_buf, A3233_TASK_LEN);

	findnonce = FALSE;
	while(1){
		AVALON_Delay(8000000);
		if((UART_Read_Cnt() >= A3233_NONCE_LEN)&& (FALSE == findnonce)){
			/* clear usb rx ring buffer */
			UCOM_FlushRxRB();

			memset(nonce_buf, 0, A3233_NONCE_LEN);
			UART_Read(nonce_buf,A3233_NONCE_LEN);

			PACK32(nonce_buf, &nonce_value);
			nonce_value -= 0x100000; /* FIXME */
			nonce_value = ((nonce_value >> 24) | (nonce_value << 24) | ((nonce_value >> 8) & 0xff00) | ((nonce_value << 8) & 0xff0000));
			UNPACK32(nonce_value, nonce_buf);

			UCOM_Write(nonce_buf,A3233_NONCE_LEN);
			findnonce = TRUE;
		}

		strcpy(dbgbuf, "cur temp = ");
		myitoa(AVALON_A3233_TemperRd(), strbuf, 10);
		strcat(dbgbuf, strbuf);
		strcat(dbgbuf,"\n");
		AVALON_USB_PutSTR(dbgbuf);

		strcpy(dbgbuf, "cur [V_25,V_18,V_CORE,V_09]=");
		myitoa(AVALON_A3233_ADCGuard(V_25), strbuf, 10);
		strcat(dbgbuf, strbuf);
		strcat(dbgbuf,",");
		myitoa(AVALON_A3233_ADCGuard(V_18), strbuf, 10);
		strcat(dbgbuf, strbuf);
		strcat(dbgbuf,",");
		myitoa(AVALON_A3233_ADCGuard(V_CORE), strbuf, 10);
		strcat(dbgbuf, strbuf);
		strcat(dbgbuf,",");
		myitoa(AVALON_A3233_ADCGuard(V_09), strbuf, 10);
		strcat(dbgbuf, strbuf);
		strcat(dbgbuf,"\n");
		AVALON_USB_PutSTR(dbgbuf);

	    strcpy(dbgbuf, "process nonce powergood = ");
		myitoa(AVALON_A3233_IsPowerGood(), strbuf, 10);
		strcat(dbgbuf, strbuf);
		strcat(dbgbuf,"\n");
		AVALON_USB_PutSTR(dbgbuf);

	}
}
