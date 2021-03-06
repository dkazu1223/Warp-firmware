#include <stdint.h>

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

volatile uint8_t	inBuffer[32];
volatile uint8_t	payloadBytes[32];


/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 13),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

static int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	OSA_TimeDelay(10);
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}



int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	enableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 13u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x0F);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0xFF);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with initialization sequence...\n");

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with enabling fill...\n");

	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with screen clear...\n");



	/*
	 *	Read the manual for the SSD1331 (SSD1331_1.2.pdf) to figure
	 *	out how to fill the entire screen with the brightest shade
	 *	of green.
	 */   

	SEGGER_RTT_WriteString(0, "\r\n\tDone with enabling fill...\n");
	
/*
// turns whole screen green
	writeCommand(0X22);
	writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(95);
        writeCommand(63);
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

*/
// turns 16/8 segment green	
/*	
	writeCommand(0X22);
	writeCommand(0x00); // start cols
        writeCommand(0x00); // start rows
        writeCommand(8); // end cols
        writeCommand(16); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);


//first seven segment test
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1); // start cols
        writeCommand(0x00); // start rows
        writeCommand(5); // end cols
        writeCommand(1); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0); // start cols
        writeCommand(2); // start rows
        writeCommand(1); // end cols
        writeCommand(5); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5); // start cols
        writeCommand(2); // start rows
        writeCommand(6); // end cols
        writeCommand(5); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1); // start cols
        writeCommand(6); // start rows
        writeCommand(5); // end cols
        writeCommand(7); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0); // start cols
        writeCommand(8); // start rows
        writeCommand(1); // end cols
        writeCommand(11); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5); // start cols
        writeCommand(8); // start rows
        writeCommand(6); // end cols
        writeCommand(11); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1); // start cols
        writeCommand(12); // start rows
        writeCommand(5); // end cols
        writeCommand(13); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	SEGGER_RTT_WriteString(0, "\r\n\tDone with draw rectangle...\n");

*/
	return 0;
}


int
devSSD1331symbols(int symbolno, int xco, int yco)
{
//Segments are 8x16
//symbolnos = numbers from 1 to 9 then 10 - 35 are the letters abc and 6969 is clear.
//Not all letters are included
int shiftx;
int shifty;
	
shiftx = xco * 8;
shifty = yco * 16;
	
//clear
if (symbolno == 6969)
	{
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0+shiftx);
	writeCommand(0+shifty);
	writeCommand(8+shiftx);
	writeCommand(16+shifty);
	}
/*	
//test 
	writeCommand(0X22);
	writeCommand(88); // start cols
        writeCommand(56); // start rows
        writeCommand(95); // end cols
        writeCommand(63); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
*/
	
//full seven segments
/*	
if (symbolno == 0)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}

	*/

if (symbolno == 0)
	{
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
       	writeCommand(0+shifty); // start rows
       	writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}	
	
else if (symbolno == 1)
	{	
	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}	
	
else if (symbolno == 2)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	
	
	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
	
else if (symbolno == 3)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}

else if (symbolno == 4)
	{	

	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	}

	
	
else if (symbolno == 5)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
else if (symbolno == 6)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
	
else if (symbolno == 7)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}

else if (symbolno == 8)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}

else if (symbolno == 9)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
else if (symbolno == 10)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	}
else if (symbolno == 31)
	{	
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}

else if (symbolno == 16)
	{	
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
	
else if (symbolno == 28)
	{	
//s
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
	
	else if (symbolno == 29)
	{	
//t
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
	
	else if (symbolno == 14)
	{	
//e
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);	

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//7 bottom segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(12+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(13+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}

	else if (symbolno == 25)
	{	
//p
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
		
	//4 middle segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(6+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(7+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);		
	
	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	}
	if (symbolno == 22)
	{
	//m
	//1 top segment	
	writeCommand(0X22);
	writeCommand(1+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
		
	//1 top segment	2
	writeCommand(0X22);
	writeCommand(1+5+shiftx); // start cols
        writeCommand(0+shifty); // start rows
        writeCommand(5+5+shiftx); // end cols
        writeCommand(1+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
		
	//3 right top segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//3 right top segment 2	
	writeCommand(0X22);
	writeCommand(5+5+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(6+5+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	
	//6 right bottom segment	
	writeCommand(0X22);
	writeCommand(5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	

	//6 right bottom segment 2	
	writeCommand(0X22);
	writeCommand(5+5+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(6+5+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}
	
	
else if (symbolno == 18)
	{
	//i
	//2 left top segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(2+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(5+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);

	//5 left bottom segment	
	writeCommand(0X22);
	writeCommand(0+shiftx); // start cols
        writeCommand(8+shifty); // start rows
        writeCommand(1+shiftx); // end cols
        writeCommand(11+shifty); // end rows
	writeCommand(0x00);
        writeCommand(0xFF);
        writeCommand(0x00);
        writeCommand(0x00);
	writeCommand(0xFF);
        writeCommand(0x00);
	}

else{
	/*writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0+shiftx);
	writeCommand(0+shifty);
	writeCommand(8+shiftx);
	writeCommand(16+shifty);*/

    }
	return 0;	
}


int
devSSD1331write(int input,int xco, int yco)
	{
int aut = input;
int d1;
int d2;
int d3;
int d4;
int df;
df = 0;
d4 = aut/1000;

if (d4 != 0)
{
	df=1;
	devSSD1331symbols(6969,xco-3,yco);
	devSSD1331symbols(d4,xco-3,yco);
}
d3 = (aut-d4*1000)/100;		
if (df!=0 || d4!=0)
{
	df=1;
	devSSD1331symbols(6969,xco-2,yco);
	devSSD1331symbols(d3,xco-2,yco);
}
d2 = (aut-d4*1000-d3*100)/10;
SEGGER_RTT_printf(0, " %d,", d2);	
if (df!=0 || d2!=0)
{
	df=1;
	devSSD1331symbols(6969,xco-1,yco);
	devSSD1331symbols(d2,xco-1,yco);	
}
d1 = (aut-d4*1000-d3*100-d2*10);	
devSSD1331symbols(d1,xco,yco);
	
	return 0;
	}
