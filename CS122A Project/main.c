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
#define SAMPLESPERFRAMECOUNTER 34
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
void Sample(){
	
}
int main(void)
{
	DDRD = 0xFF;
	DDRC = 0xFF;
	DDRA = 0xFF;
	PORTC = 0x0F;
	PORTA = 0x00;
	SPI_ServantInit();
  //  LCD_init(); 
	//LCD_DisplayString(1,"test");
	struct cpu c;
	APUInit(&(c.a));
	APUWrite(&(c.a),253,2); //440hz pulsegen init
	APUWrite(&(c.a),0x80,0); //440hz pulsegen init
	//InitPWM();
	TimerOn();
	TimerSet(1);
	InitCpu(&c);
	c.playing = 1;
	c.progcount = c.initadd;
	while (c.playing){
		TickCpu(&c);
	}
	//PORTA = 0xFF;
	c.progcount = c.playadd;
	c.playing = 1;
	unsigned char amp = 0;
	unsigned char tickcnt = 0;
	unsigned char framecountercnt = 0;
    while (1) 
    {
	    if (c.playing){
		    TickCpu(&c);
	    }
		if (TimerFlag){
			if (tickcnt < SAMPLESPERFRAMECOUNTER){
				tickcnt++;
			}
			else{
				framecountercnt++;
				tickcnt = 0;
				if (framecountercnt == 4){
					framecountercnt = 0;
					if (!c.playing){
						c.progcount = c.playadd;
						c.playing = 1;
					}
				}
			}
			/*if (pos){
				amp++;
				if (amp == 0b01111111){pos = 0;}
			}
			else{
				amp--;
				if (amp == 0x00){pos = 1;}
			}*/
			unsigned char amp = SampleAPU(&(c.a));
			if (amp < 128){
				unsigned char temp = 128 - amp;
				temp /= AMPDIV;
				amp = 128 - temp;
			}
			else{
				unsigned char temp = amp - 128;
				temp /= 2;
				amp = 128 + temp;
			}
			PORTD = amp;
			TimerFlag = 0;
		}
    }
}

