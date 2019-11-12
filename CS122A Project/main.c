/*
 * CS122A Project.c
 *
 * Created: 11/8/2019 6:13:23 PM
 * Author : jrodr0870
 
 */ 
#include <avr/io.h>
#include "lcd.h"
#include "timer.h"
#define DD_SS DDB4
#define DDR_SPI DDRB
#define DD_MOSI DDB5
#define DD_MISO DDB6
#define DD_SCK DDB7
#define SETSSOFF ((PORTB) &= ~(1 << (4)))
#define SETSSON ((PORTB) |= (1 << (4)))
volatile unsigned char spiFlag = 0;
void SPI_MasterInit(void){
	DDR_SPI = (1 << DD_MOSI) | (1<<DD_SCK) | (1<<DD_SS);
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);
}
void SPI_Transmit(unsigned char cData){
	SPDR = cData;
	while (!(SPSR & (1<<SPIF)))
	;
}
void SPI_ServantInit(void){
	sei();
	DDR_SPI = (1<<DD_MISO);
	SPCR = (1<<SPE) | (1<<SPIE);
}
void displayReceived(unsigned char rx){
	char str[4];
	str[3] = 0;
	for (unsigned int i = 0; i < 3; i++){
		str[2-i] = rx%10 + '0';
		rx = rx/10;
	}
	LCD_DisplayString(1, &str);
}
unsigned char SPI_ServantReceive(void)
{
	while (!(SPSR & (1<<SPIF)))
	;
	return SPDR;
}
ISR(SPI_STC_vect){
	spiFlag = 1;
}

int main(void)
{
	DDRD = 0xFF;
	DDRA = 0xFF;
	SPI_ServantInit();
    LCD_init();
	LCD_DisplayString(1,"test");
	//displayReceived(0xAB);
    while (1) 
    {
		if (spiFlag){
			unsigned char rx = SPI_ServantReceive();
			displayReceived(rx);
			SPI_Transmit(rx);
			spiFlag = 0;
		}
    }
}

