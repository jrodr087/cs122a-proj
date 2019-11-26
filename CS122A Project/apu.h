/*
 * apu.h
 *
 * Created: 11/18/2019 8:03:29 PM
 *  Author: jrodr087
 */ 


#ifndef APU_H_
#define APU_H_
#define apuregsize 0x0018
#define aputop 0x4000

struct pulsegen{
	unsigned char regs[4]
};
struct apu{
	unsigned char ce;//channel enable and length counter
	unsigned char framecounter;//framecounter
	struct pulsegen pulse1;
	struct pulsegen pulse2;
	
};
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

#endif /* APU_H_ */