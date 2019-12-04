/*
 * apu.h
 *
 * Created: 11/18/2019 8:03:29 PM
 *  Author: jrodr087
 */ 


#ifndef APU_H_
#define APU_H_
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#define apuregsize 0x0018
#define aputop 0x4000
#define timermask 0x20 //bitmask for the length counter halt
#define CPUCLOCK 1789773.0
#define SAMPLERATE 8192 //8.192khz sample rate
#define ANGLEPERSTEP 2 //equivalent to TRIGINT_ANGLES_PER_CYCLE/SAMPLERATE
#define harmonics 6 //how many additions of sine do we want
#define PI 3.14159

struct pulsegen{
	uint8_t regs[4];
};

struct apu{
	uint8_t ce;//channel enable and length counter
	uint8_t framecounter;//framecounter, not actually emulating it clock accurate just using it to count the 4 steps
	float currangle;//current angle for the sine
	struct pulsegen pulse1;
	struct pulsegen pulse2;
	
};
void APUInit(struct apu *a){
	a->ce = 0x0F;
	a->framecounter = 0;
	a->currangle = 0.0;
	for (uint8_t i = 0; i < 4; i++){
		a->pulse1.regs[i] = 0;
		a->pulse2.regs[i] = 0;
	}
}
void APUWrite(struct apu *a, uint8_t val, uint8_t reg){
	if (reg < 4){//pulse wave generator 1
		a->pulse1.regs[reg] = val;
	}
	else if (reg < 8){//pulse wave 2
		a->pulse1.regs[reg - 4] = val;
	}
	else if (reg < 0x0B){//triangle generator
		
	}
	else if (reg == 0x15){//channel enable and elngth counter
		a->ce = val;
	}
	else if (reg == 0x017){//frame counter
		a->framecounter = val;
	}
}

void APUFrameStep(struct apu *a){//should be run at 240hz clock
	if (a->framecounter == 0x03){a->framecounter= 0;}//after the 4th step we loop back to 0
	else{a->framecounter++;}//increment frame counter
	if (a->framecounter%2 == 0){//every second frame increment
		//clock length counters and sweep units
	}
	//clock envelopes and triangle linear counter
	if (!(a->pulse1.regs[0] & timermask)){//if we arent halting pulse1's timer
		//decrement lenght counter
	}
		
}
float approxsin(float t){
	float j = t*.15915;
	j = j - (int)j;
	return 20.785 * j * (j - 0.5) * (j - 1.0f); 
}
float SampleAPUSquare1(struct apu *a, double sampleTime){
	uint16_t t = ((a->pulse1.regs[3] & 0x07) << 8) + a->pulse1.regs[2];//returns the timer
	uint8_t duty = (a->pulse1.regs[0] >> 6) & 0x03;
	double dutycycle;
	switch (duty){
		case 0x00:
			dutycycle = 1.0/8.0;
			break;
		case 0x01:
			dutycycle = 1.0/4.0;
			break;
		case 0x02:
			dutycycle = 1.0/2.0;
			break;
		case 0x03:
			dutycycle = 3.0/4.0;
			break;
		default:
			dutycycle = .5;
			break;
	}
	dutycycle = dutycycle * 2 * PI;
	if (t < 8){return 0.0;}
	float f = CPUCLOCK/(16* (t+1));//magic calculation im getting off of http://wiki.nesdev.com/w/index.php/APU_Misc
	printf("frequency sample: %f , duty cycle: %f \n", f, dutycycle);
	float amp = 0.0;
	float a1 = 0;
	float b = 0;
	for (uint8_t i = 1; i <= harmonics; i++){
		double c = i*f*2.0*PI*sampleTime;
		a1 += (sin(c)/i)/4.0;
		b += (sin(c - dutycycle*i)/i)/4.0;
	}
	amp = (2.0/PI)* (a1 - b);
	return amp;
}
uint8_t NaiveSampleSquare(struct apu* a, double sampleTime){
	uint16_t t = ((a->pulse1.regs[3] & 0x07) << 8) + a->pulse1.regs[2];//returns the timer
	uint8_t duty = (a->pulse1.regs[0] >> 6) & 0x03;
	double dutycycle;
	switch (duty){
		case 0x00:
			dutycycle = 1.0/8.0;
			break;
		case 0x01:
			dutycycle = 1.0/4.0;
			break;
		case 0x02:
			dutycycle = 1.0/2.0;
			break;
		case 0x03:
			dutycycle = 3.0/4.0;
			break;
		default:
			dutycycle = .5;
			break;
	}
	dutycycle = dutycycle * 2 * PI;
	if (t < 8){return 0.0;}
	float f = CPUCLOCK/(16* (t+1));//magic calculation im getting off of http://wiki.nesdev.com/w/index.php/APU_Misc
	//printf("frequency sample: %f , duty cycle: %f \n", f, dutycycle);
	float angle = f*sampleTime*2*PI;
	angle = fmod(angle,2*PI);
	if (angle < dutycycle){
		return 0xFF;
	}
	else{
		return 0x00;
	}
}
#endif /* APU_H_ */