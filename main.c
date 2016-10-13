#include "samd11.h"
#define NOP() __asm volatile ( "NOP" )
#define pinHigh(pin) *(unsigned int*) (BasePortA + OffsetOut) |= (1<<pin)
#define pinLow(pin) *(unsigned int*) (BasePortA + OffsetOut) &= ~(1<<pin)
	
#define BasePortA 0x41004400
#define OffsetOut 0x10
#define OffsetDir 0x00
#define OffsetPMUX 0x30
#define OffsetPINCFG 0x40
#define OffsetIN 0x20

#define BasePM 0x40000400
#define OffsetAPBCMASK 0x20

#define BaseGCLK 0x40000C00
#define OffsetGENDEV 0x8
#define OffsetGENCTRL 0x4
#define OffsetCLKCTRL 0x2

#define BaseSERCOM0 0x42000800
#define OffsetCTRLA 0x0
#define OffsetCTRLB 0x4
#define OffsetBAUD 0xC
#define OffsetDATA 0x28
#define OffsetINTFLAG 0x18

#define BaseADC 0x42002000
#define OffsetADCCTRLB 0x04
#define OffsetADCCTRLA 0x00
#define OffsetINPUTCTRL 0x10
#define OffsetRESULT 0x1A
#define OffsetSWTRIG 0x0C

void nopDelay(unsigned long delay);
void putcharSERCOM0(void);

int main()
{
	unsigned int input;
	//Chapter 14 of manual
	//Setup GCLK 1 (generic clock 1) from int source mux it to SERCOM1; internal source is 8 MHz/8 = 1 MHz = f_GCLK1	
	*(unsigned int*) (BaseGCLK + OffsetGENDEV) = 0x1; 								// divide by 1
	*(unsigned int*) (BaseGCLK + OffsetGENCTRL) = (1<<16) | (0x6 <<8 ) | (1<<0); 	// enable clock generator, OSC8M source, GCLKGen1
	*(unsigned short*) (BaseGCLK + OffsetCLKCTRL) = (1<<14) | (1<<8) | (0x0E<<0); 	// enable clk, GCLK 1, SERCOM0_CORE
	*(unsigned int*) (BaseGCLK + OffsetGENDEV) = 0x1; 		
	*(unsigned int*) (BaseGCLK + OffsetGENCTRL) = (1<<16) | (0x6 <<8 ) | (2<<0);
	*(unsigned short*) (BaseGCLK + OffsetCLKCTRL) = (1<<14) | (2<<8) | (0x13<<0);
	//Chapter 15 of manual
	*(unsigned int*) (BasePM + OffsetAPBCMASK) |= (1<<2);			//Turn on SERCOM0 APB clock
	*(unsigned int*) (BasePM + OffsetAPBCMASK) |= (1<<8);			//Turn on ADC APB clock
	
	//Chapter 22
	
	//Setup LED pin (PA4): (And pulse output)
	PORT->Group->DIR.reg |= (1<<16);
	PORT->Group->DIR.reg |= (1<<6);
	
	//Setup Tx/Rx for USART, using pin PA14 for Tx:
	*(unsigned int*) (BasePortA + OffsetDir) |= (1<<8);
	//DIR register set PA14 to output
	*(unsigned char*) (BasePortA + OffsetPMUX + 4) = 0x33;
	//PA14 and PA15 to group C
	*(unsigned char*) (BasePortA + OffsetPINCFG + 8) |= 0x1;
	//Enable Pinmux for PA14
	*(unsigned char*) (BasePortA + OffsetPINCFG + 9) |= 0x1;
	//Enable Pinmux for PA15
	
	//Set PA02, PA03, PA04, PA05 to inputs
	PORT->Group->DIR.reg &= ~(1<<2);
	PORT->Group->DIR.reg &= ~(1<<3);
	PORT->Group->DIR.reg &= ~(1<<4);
	PORT->Group->DIR.reg &= ~(1<<5);
	*(unsigned char*) (BasePortA + OffsetPINCFG + 2) |= 0x2;
	*(unsigned char*) (BasePortA + OffsetPINCFG + 3) |= 0x2;
	*(unsigned char*) (BasePortA + OffsetPINCFG + 4) |= 0x2;
	*(unsigned char*) (BasePortA + OffsetPINCFG + 5) |= 0x2;
	
	//Chapter 24, and 25
	//Setup the SERCOM0 as UART with 115200-1-N with 1 MHz source clock
	//Configure CTRLA: LSB first, SERCOM Pad 1, 8x oversampled/fractional Baud, internal clock
	*(unsigned int*) (BaseSERCOM0 + OffsetCTRLA) |= (1<<30) | (3<<20) | (1<<16) | (3<<13) | (1<<2);
	//Set Baud: Fractional part 1, Whole part: 1; result 1 + 1/8
	*(unsigned short*) (BaseSERCOM0 + OffsetBAUD) = (1<<13) | (1<<0);
	*(unsigned int*) (BaseSERCOM0 + OffsetCTRLB) |= (1<<16) | (1<<17); //Enable TX and RX
	*(unsigned int*) (BaseSERCOM0 + OffsetCTRLA) |= (1<<1);
	//Enable UART	
	
	*(unsigned short*) (BaseADC + OffsetADCCTRLB) |= (3<<4);
	*(unsigned char*) (BaseADC + OffsetADCCTRLA) |= (1<<2) | (1<<1);
	*(unsigned int*) (BaseADC + OffsetINPUTCTRL) |= (0x18<<8) | (0x3);
		
 while(1)
 {
	 input = *(unsigned int*) 0x41004420;
	 if((input & 0x3C)== 0x14) {
			nopDelay(1);
			pinHigh(6);
			pinHigh(16);
			nopDelay(1);
			pinLow(6);
			pinLow(16);
	 }
 }
}

//Output a single character onto SERMCOM0
//This function is blocking and will wait forever for the UART to become available
void putcharSERCOM0(void)
{
	volatile unsigned char flag;
	volatile unsigned char result;
	
	*(unsigned char*) (BaseADC + OffsetSWTRIG) |= (1<<1);
	result = *(unsigned char*) (BaseADC + OffsetRESULT);
	
	do
	{
		flag = *(unsigned char*) (BaseSERCOM0 + OffsetINTFLAG);
	}while((flag & 0x1) != 0x1);
	

	*(unsigned short*) (BaseSERCOM0 + OffsetDATA) = result; //output character code

}

// Burn CPU cycles and spin doing nothing
void nopDelay(unsigned long delay)
{
 unsigned long i, j;
 for(j = 0; j<delay; j++)
 {
 for(i = 0; i<0xFFF; i++)NOP();
 }
}
