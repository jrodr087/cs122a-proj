/*
 * apu.h
 *
 * Created: 11/18/2019 8:03:29 PM
 *  Author: jrodr087
 */ 


#ifndef APU_H_
#define APU_H_
#include "trigintlib/trigint.h"
#define apuregsize 0x0018
#define aputop 0x4000
#define timermask 0x20 //bitmask for the length counter halt
#define CPUCLOCK 1789773
#define SAMPLERATE 8192 //8.192khz sample rate
#define ANGLEPERSTEP 2 //equivalent to TRIGINT_ANGLES_PER_CYCLE/SAMPLERATE
#define harmonics 4 //how many additions of sine do we want

struct pulsegen{
	unsigned char regs[4];
};

struct apu{
	unsigned char ce;//channel enable and length counter
	unsigned char framecounter;//framecounter, not actually emulating it clock accurate just using it to count the 4 steps
	trigint_angle_t currangle;//current angle for the sine
	struct pulsegen pulse1;
	struct pulsegen pulse2;
	
};
void APUInit(struct apu *a){
	a->ce = 0;
	a->framecounter = 0;
	a->currangle = 0;
	for (unsigned char i = 0; i < 4; i++){
		a->pulse1.regs[i] = 0;
		a->pulse2.regs[i] = 0;
	}
}
void APUWrite(struct apu *a, unsigned char val, unsigned char reg){
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
unsigned char SampleAPU(struct apu *a){
	unsigned short t = ((a->pulse1.regs[3] & 0x07) << 8) + a->pulse1.regs[2];//returns the timer
	if (t < 8){return 128;}
	unsigned short f = CPUCLOCK/(16* (t+1));//magic calculation im getting off of http://wiki.nesdev.com/w/index.php/APU_Misc
	unsigned short angleinc = f*ANGLEPERSTEP;//this will be off slightly probably
	a->currangle += angleinc;
	unsigned char amp = 128;
	for (unsigned char i = 0; i < harmonics; i++){
		unsigned char harm = (i*2)+1;
		trigint_angle_t angle = a->currangle*(harm);
		angle = angle & TRIGINT_ANGLE_MAX;
		unsigned char tempamp = trigint_sin8u(angle);
		if (tempamp < 128){
			tempamp = (128 - tempamp);
			tempamp /= harm;
			amp -= tempamp;
		}
		else{
			tempamp = tempamp - 128;
			tempamp /= harm;
			amp += tempamp;
		}
	}
	return amp;
}
#endif /* APU_H_ */