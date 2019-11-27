/*
 * cpu.h
 *
 * Created: 11/18/2019 3:55:33 PM
 *  Author: jrodr087
 */ 


#ifndef CPU_H_
#define CPU_H_
#include "spi.h"
#include "apu.h"
#define nbitpos 7
#define vbitpos 6
#define bbitpos 4
#define dbitpos 3
#define ibitpos 2
#define zbitpos 1
#define cbitpos 0
#define stackhead 0x0100
#define mirrorhead 0x01FF//ram is mirrored so that the address at 0x0001 == 0x0800
#define ramsize 0x0800//size of ram (2048bytes)
#define FETCH 0x01 //this is the spi instruction to send in order to receive an instruction from rpi
#define READ 0x02 //this is the spi instruction to read an address from rpi
#define WRITE 0x03 //this is the spi instruction to write to an address from rpi
#define INITEMU 0x04 //this is the spi instruction to tell the rpi to init the emulation
#define FUNCTIONBUFFSIZE 256 //size of the buffers for the init and play functions
enum CPUStatus {
	init,
	waitrpi,
	running
	};
struct cpu
{
	unsigned char s; //stack pointer
	unsigned char p; //status register
	/* bit of the status register in order starting from bit 7 down
	N: negative flag set when any arithmetic ops give a negative
	V: overflow flag set on a sign overflow on signed character operations clear otherwsise
	1 unused always 1
	B: break flag always set except when p is pushed on the stack when jumping
	D: decimal mode flag set to select the binary coded decimal mode, typicaly 0
	I: interrupt disable flag set to disable jumping to the IRQ handler vector. automatically set when jumping to the IRQ handler to prevent blocking
	Z:  zero flag set when an arithmetic operation is loaded with the value 0 and clear otherwise
	C: carry flag used in additions and subtractions as a 9th bit
	*/
	unsigned char acc; //accumulator
	unsigned char x; //indexing register x
	unsigned char y; // indexing register y
	unsigned char temp; //this is testing stuff for the spi
	
	unsigned char instbuffer[3]; //buffer for instructions read from spi
	unsigned char RAM[ramsize];//2KB internal ram
	unsigned char initbuff[FUNCTIONBUFFSIZE];//internal buffer for the init function
	unsigned char playbuff[FUNCTIONBUFFSIZE];//internal buffer for the play function
	unsigned short pc; // program counter
	unsigned short playspeed;
	unsigned short playadd;
	unsigned short initadd;
	
	unsigned short clocks;
	enum CPUStatus state;
	struct apu a;
};
void PushStack(struct cpu* c, unsigned char val){
	c->s--;
	c->RAM[c->s+stackhead] = val;
}
unsigned char PopStack(struct cpu* c){
	c->s++;
	return c->RAM[c->s-1 + stackhead];
}
unsigned char PeekStack(struct cpu* c){
	return c->RAM[c->s+stackhead];
}
void InitCpu(struct cpu* c){
	c->s = 0xFF;//stack grows downwards
	c->p = 0;
	c->acc = 0;
	c->x = 0;
	c->y = 0;
	c->instbuffer[0]=0;
	c->instbuffer[1]=0;
	c->instbuffer[2]=0;
	c->initadd = SPI_ServantReceive();
	c->initadd += SPI_ServantReceive() << 8;
	c->playadd = SPI_ServantReceive();
	c->playadd += SPI_ServantReceive() << 8;
	c->playspeed = SPI_ServantReceive();
	c->playspeed += SPI_ServantReceive() << 8;
	c->pc = c->initadd;
	c->clocks = 0;
	c->state = running;
}
unsigned char LoadFromBus(unsigned short pos){
	//////should read a given address from the rpi
	return 0x00;
}
void WriteToBus(unsigned short pos, unsigned char val){
	////should write to a given address on the rpi
	return;
}
unsigned char ReadMemory(struct cpu *c, unsigned short pos){
	if (pos <= mirrorhead){
		pos = pos % ramsize;
		return c->RAM[pos];
	}
	else if (pos >= c->playadd && pos < c->playadd+256){
		return c->playbuff[pos-c->playadd];	
	}
	else if (pos >= c->initadd && pos < c->initadd+256){
		return c->initbuff[pos-c->initadd];
	}
	return 0x00;
}
void PopulateBuffers(struct cpu *c){//fills the play and init buffers so we dont have to spi so much
	c->pc = c->initadd;
	for (unsigned char i = 0; i < 255;i++){
		SPI_Transmit(READ);
		SPI_Transmit((c->pc >>8) & 0xFF);
		SPI_Transmit(c->pc & 0xFF);
		c->initbuff[i] = SPI_ServantReceive();
		c->pc++;
	}//get the first 255 bytes for the init buffer
	SPI_Transmit(READ);
	SPI_Transmit((c->pc >>8) & 0xFF);
	SPI_Transmit(c->pc & 0xFF);
	c->initbuff[255] = SPI_ServantReceive();//gets the last byte for the buffer
	c->pc = c->playadd;
	for (unsigned char i = 0; i < 255;i++){
		SPI_Transmit(READ);
		SPI_Transmit((c->pc >>8) & 0xFF);
		SPI_Transmit(c->pc & 0xFF);
		c->playbuff[i] = SPI_ServantReceive();
		c->pc++;
	}//get the first 255 bytes for the play buffer
	SPI_Transmit(READ);
	SPI_Transmit((c->pc >>8) & 0xFF);
	SPI_Transmit(c->pc & 0xFF);
	c->initbuff[255] = SPI_ServantReceive();//gets the last byte
}
void WriteMemory(struct cpu *c, unsigned short pos, unsigned char val){//writes to certain memory addresses
	if (pos <= mirrorhead){//write to wam
		pos = pos % ramsize;
		c->RAM[pos] = val;
		return;
	}
	if (pos >= 0x4000 && pos <= 0x4017){//apu write
		APUWrite(&(c->a),val,pos & 0xFF);
	}
	
}
void FetchInstruction(struct cpu* c){
	if (c->pc >= c->initadd && c->pc < c->initadd+FUNCTIONBUFFSIZE-3){
		c->instbuffer[0] = c->initbuff[c->pc-c->initadd];
		c->instbuffer[1] = c->initbuff[c->pc-c->initadd+1];
		c->instbuffer[2] = c->initbuff[c->pc-c->initadd+2];
		return;
	}
	if (c->pc >= c->playadd && c->pc < c->playadd+FUNCTIONBUFFSIZE-3){
		c->instbuffer[0] = c->initbuff[c->pc-c->playadd];
		c->instbuffer[1] = c->initbuff[c->pc-c->playadd+1];
		c->instbuffer[2] = c->initbuff[c->pc-c->playadd+2];
		return;
	}
	SPI_Transmit(FETCH);
	SPI_Transmit((c->pc >>8) & 0xFF);
	SPI_Transmit(c->pc & 0xFF);
	c->instbuffer[0] = SPI_ServantReceive();
	c->instbuffer[1] = SPI_ServantReceive();
	c->instbuffer[2] = SPI_ServantReceive();
}
void RunInstruction(struct cpu* c){
	FetchInstruction(c);
	switch (c->instbuffer[0]){//implementing this sucks
		case 0x00: //BRK impl
			c->pc+= 2;
			break;
		case 0x01://OR oper,x
			c->pc+= 2;
			break;
		case 0x05://OR oper
			c->pc+= 2;
			break;
		case 0x06://ASL zpg
			c->pc+= 2;
			break;
		case 0x08://PHP impl
			c->pc+= 1;
			break;
		case 0x09://OR #oper
			c->pc+= 2;
			break;
		default://nop
			c->pc+= 2;
			break;
	}
}

void TickCpu(struct cpu* c){
	switch (c->state){
		case waitrpi:
			SPI_ServantReceive();
			spiFlag = 0;
			c->state = running;
			break;
		case init:
			InitCpu(c);
			PopulateBuffers(c);
			c->state = waitrpi;
			break;
		case running:
			RunInstruction(c);
			break;
	}
}
void SetPBit(struct cpu* tmp, unsigned char pos){
	tmp->p |= ( 0x01 << pos);
}
void ClearPBit(struct cpu* tmp, unsigned char pos){
	tmp->p &= ~(0x01 << pos);
}
#endif /* CPU_H_ */