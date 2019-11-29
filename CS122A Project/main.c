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
#define AMPDIV 2
unsigned char currindex = 0;
void displayReceived(unsigned char rx){
	unsigned char str[4];
	str[3] = 0;
	for (unsigned int i = 0; i < 3; i++){
		str[2-i] = rx%10 + '0';
		rx = rx/10;
	}
	LCD_DisplayString(1, &str);
}
void InitPWM(){
	TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0A0);
	TCCR0B = (1<<CS00) | (1<<WGM02);
	TCNT0 = 0;
	OCR0A = 128;
	DDRB|=(1<<DDB3);
}

int main(void)
{
	DDRD = 0xFF;
	DDRC = 0xFF;
	DDRA = 0xFF;
	PORTC = 0x0F;
	SPI_ServantInit();
  //  LCD_init(); 
	//LCD_DisplayString(1,"test");
	struct cpu c;
	APUInit(&(c.a));
	APUWrite(&(c.a),253,2); //440hz pulsegen init
	//InitPWM();
	TimerOn();
	//TimerSet(1);
	//InitCpu(&c);
	//unsigned char rx;
	//srand(1);
	//displayReceived(0xAB);
    while (1) 
    {
		if (TimerFlag){
			unsigned char amp = SampleAPU(&(c.a));
// 			if (amp < 128){
// 				unsigned char temp = 128 - amp;
// 				temp /= AMPDIV;
// 				amp = 128 - temp;
// 			}
// 			else{
// 				unsigned char temp = amp - 128;
// 				temp /= 2;
// 				amp = 128 + temp;
// 			}
			PORTD = amp;
			TimerFlag = 0;
		}
    }
}

