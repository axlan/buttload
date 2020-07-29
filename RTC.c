//*****************************************************************************
//
//  File........: RTC.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: IAR EWAAVR 4.20a
//
//  Description.: Real Time Clock (RTC)
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - Created                                       - LHM
//
//*****************************************************************************

//  Include files
#include "main.h"
#include "RTC.h"
#include "LCD_Driver.h"


/*****************************************************************************
*
*   Function name : CHAR2BCD2
*
*   Returns :       Binary coded decimal value of the input (2 digits)
*
*   Parameters :    Value between (0-99) to be encoded into BCD
*
*   Purpose :       Convert a character into a BCD encoded character.
*                   The input must be in the range 0 to 99.
*                   The result is byte where the high and low nibbles
*                   contain the tens and ones of the input.
*
*****************************************************************************/
char CHAR2BCD2(char input)
{
	char high = 0;
	
	
	while (input >= 10)                 // Count tens
	{
		high++;
		input -= 10;
	}

	return  (high << 4) | input;        // Add ones and return answer
}

/*****************************************************************************
*
*   Function name : CHAR2BCD3
*
*   Returns :       Binary coded decimal value of the input (3 digits)
*
*   Parameters :    Value between (0-255) to be encoded into BCD
*
*   Purpose :       Convert a character into a BCD encoded character.
*                   The input must be in the range 0 to 255.
*                   The result is an integer where the three lowest nibbles
*                   contain the ones, tens and hundreds of the input.
*
*****************************************************************************/
unsigned int CHAR2BCD3(char input)
{
	int high = 0;
	
	while (input >= 100)                // Count hundreds
	{
		high++;
		input -= 100;
	}

	high <<= 4;
	
	while (input >= 10)                 // Count tens
	{
		high++;
		input -= 10;
	}

	return  (high << 4) | input;        // Add ones and return answer
}


void ShowTime(uint32_t totalSeconds)
{
	char HH, HL, MH, ML, SH, SL;

	uint8_t gHOUR = totalSeconds / (60 * 60);
	uint8_t gMINUTE = (totalSeconds - (gHOUR * 60)) / 60;
	uint8_t gSECOND = totalSeconds % 60;
	
	
	
	HH = CHAR2BCD2(gHOUR);
	
	HL = (HH & 0x0F) + '0';
	HH = (HH >> 4) + '0';

	MH = CHAR2BCD2(gMINUTE);
	ML = (MH & 0x0F) + '0';
	MH = (MH >> 4) + '0';

	SH = CHAR2BCD2(gSECOND);
	SL = (SH & 0x0F) + '0';
	SH = (SH >> 4) + '0';
	
	char str[] = {HH, HL, '-', MH, ML, '-', SH, SL, '\0'};

	LCD_PutStr(str);

}

void ShowTimeMinutes(uint32_t totalSeconds, char seperator)
{
	char HH, HL, MH, ML;

	uint16_t totalMinutes = totalSeconds / 60UL;
	
	uint8_t gHOUR = totalMinutes / 60;
	uint8_t gMINUTE = totalMinutes % 60;
	
	HH = CHAR2BCD2(gHOUR);
	
	HL = (HH & 0x0F) + '0';
	HH = (HH >> 4) + '0';

	MH = CHAR2BCD2(gMINUTE);
	ML = (MH & 0x0F) + '0';
	MH = (MH >> 4) + '0';

	char str[] = {HH, HL, seperator, MH, ML, '\0'};

	LCD_PutStr(str);
}
