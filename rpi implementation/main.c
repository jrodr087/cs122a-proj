/*
 * CS122A Project.c
 *
 * Created: 11/8/2019 6:13:23 PM
 * Author : jrodr0870
 
 */
 #include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <bcm2835.h>
#include <signal.h>
 #include "cpu.h"
#define AMPDIV 2
#define SAMPLESPERFRAMECOUNTER 34
#define SECONDSPERPLAYCALL .01666
#define SECONDSPERSAMPLE .0000625
static volatile int keepRunning = 1;
void intHandler(int dummy){
	keepRunning = 0;
}
int InitSPI(){
	if (!bcm2835_init()){
		return 1;
	}
	if (!bcm2835_spi_begin()){
		return 1;
	}
	signal(SIGINT,intHandler);
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_1024);
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0,LOW);
	bcm2835_gpio_fsel(18,BCM2835_GPIO_FSEL_ALT5 ); //PWM0 signal on GPIO18    
    bcm2835_gpio_fsel(13,BCM2835_GPIO_FSEL_ALT0 ); //PWM1 signal on GPIO13  
	bcm2835_pwm_set_clock(2);
	bcm2835_pwm_set_mode(0, 1, 1); //channel 0, markspace mode, PWM enabled.
    bcm2835_pwm_set_mode(1, 1, 1); //channel 1, markspace mode, PWM enabled. 
	return 0;
}


int main(void)
{
	if (InitSPI()){return 1;}
	struct cpu c;
	APUInit(&(c.a));
	
	InitCpu(&c);
	c.playing = 1;
	c.progcount = c.initadd;
	c.acc = 2;
	c.x = 0;
	c.y = 0x00;
	//clock_t start, end, startSample, endSample;
	struct timeval start,end, total, startsample,endsample,totalsample;
	double cpu_time_used;
	//start = clock();
	gettimeofday(&start,NULL);
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
	//end = clock();
	gettimeofday(&end,NULL);
	timersub(&end,&start,&total);
	cpu_time_used = total.tv_sec + (total.tv_usec*.000001f);//((double)(end-start))/CLOCKS_PER_SEC;
	printf("time taken for init: %f\n",cpu_time_used);
	c.progcount = c.playadd;
	c.playing = 1;
	uint8_t amp = 0;
	uint8_t tickcnt = 0;
	uint8_t framecountercnt = 0;
	cpu_time_used = 0;
	float sampletime = 0.0;
	double globalSampleTime = 0.0;
	gettimeofday(&startsample,NULL);
    while (keepRunning)  
    {
		gettimeofday(&start,NULL);
		if (c.playing){
			TickCpu(&c);
		}
		gettimeofday(&end,NULL);
		timersub(&end,&start,&total);
		//end = clock();
		cpu_time_used += total.tv_sec + (total.tv_usec*.000001f);
		if (cpu_time_used > SECONDSPERPLAYCALL){
			if (!c.playing){
				printf("finished a playcall at %f seconds\n", cpu_time_used);
				c.playing = 1;
				c.progcount = c.playadd;
				cpu_time_used = 0.0;
			}
		}
		gettimeofday(&endsample,NULL);
		timersub(&endsample,&startsample,&totalsample);
		sampletime += total.tv_sec + (total.tv_usec*.000001f);
		if (sampletime >= SECONDSPERSAMPLE){
			globalSampleTime += sampletime;
			float samplerec = SampleAPUSquare1(&(c.a), globalSampleTime);
			if (samplerec > 1.0){samplerec = 1.0;}
			if (samplerec < -1.0){samplerec = -1.0;}
			int8_t sample8bit = samplerec *128;
			sample8bit += 128;
			//printf("sample taken: %d \n", sample8bit);
			//bcm2835_spi_transfer(sample8bit);
			//bcm2835_pwm_set_data(0,sample12bit>>6 & 0xFF);
			//bcm2835_pwm_set_data(1,sample12bit & 0xFF);
			sampletime = 0.0;
		}
		gettimeofday(&startsample,NULL);
    }
	bcm2835_spi_end();
	bcm2835_close();
}

