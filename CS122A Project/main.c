/*
 * CS122A Project.c
 *
 * Created: 11/8/2019 6:13:23 PM
 * Author : jrodr0870
 
 */
#include <avr/io.h>
#include "lcd.h"
#include "timer.h"
#include "cpu.h"
unsigned char currindex = 0;
//unsigned char vals[255];
void displayReceived(unsigned char rx){
	unsigned char str[4];
	str[3] = 0;
	for (unsigned int i = 0; i < 3; i++){
		str[2-i] = rx%10 + '0';
		rx = rx/10;
	}
	LCD_DisplayString(1, &str);
}

int main(void)
{
	DDRD = 0xFF;
	DDRC = 0xFF;
	DDRA = 0xFF;
	PORTC = 0x0F;
	SPI_ServantInit();
    LCD_init(); 
	LCD_DisplayString(1,"test");
	struct cpu c;
	InitCpu(&c);
	//unsigned char rx;
	//srand(1);
	//displayReceived(0xAB);
    while (1) 
    {
		TickCpu(&c);
    }
}

