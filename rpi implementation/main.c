/*
 * CS122A Project.c
 *
 * Created: 11/8/2019 6:13:23 PM
 * Author : jrodr0870
 
 */
#include <stdint.h>
#include <time.h>
 #include "cpu.h"
#define AMPDIV 2
#define SAMPLESPERFRAMECOUNTER 34
int main(void)
{
  //  LCD_init(); 
	//LCD_DisplayString(1,"test");
	struct cpu c;
	APUInit(&(c.a));
	APUWrite(&(c.a),253,2); //440hz pulsegen init
	APUWrite(&(c.a),0x80,0); //440hz pulsegen init
	APUWrite(&(c.a),0x7E,6); //880hz pulsegen init
	APUWrite(&(c.a),0x80,0); //880hz pulsegen init
	
	InitCpu(&c);
	c.playing = 1;
	c.progcount = c.initadd;
	c.acc = 2;//c.startingsong-1;
	c.x = 0;
	c.y = 0x00;
	clock_t start, end;
	double cpu_time_used;
	start = clock();
	while (c.playing){
		TickCpu(&c);
	}
	unsigned short pos = 0;
	/*while (pos < 0x0800){
		for (unsigned char i = 0; i < 0x10; i++){
			printf("%02X",c.RAM[pos+i]);
			printf(" ");
		}
		printf("\n");
		pos += 0x0010;
	}*/
	end = clock();
	cpu_time_used = ((double)(end-start))/CLOCKS_PER_SEC;
	printf("time taken: %f\n",cpu_time_used);
	c.progcount = c.playadd;
	c.playing = 1;
	uint8_t amp = 0;
	uint8_t tickcnt = 0;
	uint8_t framecountercnt = 0;
    while (1)  
    {
		c.playing = 1;
		c.progcount = c.playadd;
		start = clock();
		while (c.playing){
			TickCpu(&c);
		}
		end = clock();
		cpu_time_used = ((double)(end-start))/CLOCKS_PER_SEC;
		printf("time taken: %f\n",cpu_time_used);
	   /* if (c.playing){
		    TickCpu(&c);
	    }*/
			/*if (tickcnt < SAMPLESPERFRAMECOUNTER){
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
			}*/
			uint8_t amp = SampleAPUSquare1(&(c.a));
			if (amp < 128){
				uint8_t temp = 128 - amp;
				temp /= AMPDIV;
				amp = 128 - temp;
			}
			else{
				uint8_t temp = amp - 128;
				temp /= 2;
				amp = 128 + temp;
			}
		
    }
}

