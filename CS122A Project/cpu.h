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
#define CFLAG 0
#define ZFLAG 1
#define IFLAG 2
#define DFLAG 3
#define BFLAG 4
#define VFLAG 6
#define NFLAG 7
enum CPUStatus {
	init,
	waitrpi,
	running
	};
struct cpu
{
	unsigned char s; //stack pointer
	unsigned char status; //status register
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
	signed char acc; //accumulator
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
inline void SetPBit(struct cpu* tmp, unsigned char pos, unsigned char val){
	if (val){
		tmp->status |= ( 0x01 << pos);
	}
	else{
		tmp->status &= ~(0x01 << pos);
	}
}
unsigned char GetPBit(struct cpu* c,unsigned char pos){
	return ((c->status >> pos) & 0x01);
}
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
	c->status = 0;
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
void ADCFunction(struct cpu* c, unsigned char inc){
	unsigned short sum;
	sum = c->acc + inc + ((c->status >> CFLAG) & 0x01);
	if (sum >= 0xFF){
		SetPBit(c,CFLAG,1);
	}
	else{
		SetPBit(c,CFLAG,0);
	}
	if ((inc & 0x80 && c->acc & 0x80) && !(sum && 0x80)){//if the inc and acc vals are both negative and the sum isnt then overflow
		SetPBit(c,VFLAG,1);
	}
	else if (!(inc & 0x80 || c->acc & 0x80) && sum && 0x80){//if the inc and acc vals are both positive and the sum isnt
		SetPBit(c,VFLAG,1);
	}
	else{SetPBit(c,VFLAG,0);}
	c->acc = sum & 0xFF;
	SetPBit(c,NFLAG,c->acc & 0x80);
	SetPBit(c,ZFLAG, c->acc == 0x00);
}
void RunSubset1Instructions(struct cpu* c, unsigned char op, unsigned char amode){
	unsigned char cpos = 0;//used for zero page addressing
	unsigned short spos = 0;//used for addressing
	signed char temp = 0; //used for temporary math
	switch (op){
		case 0x00://ORA
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x01://zeropage
			c->acc = c->acc | c->RAM[c->instbuffer[1]];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x02://immediate
			c->acc = c->acc | c->instbuffer[1];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = c->acc | c->RAM[cpos];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		case 0x01://AND
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x01://zeropage
			c->acc = c->acc & c->RAM[c->instbuffer[1]];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x02://immediate
			c->acc = c->acc & c->instbuffer[1];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = c->acc & c->RAM[cpos];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		case 0x02://EOR
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x01://zeropage
			c->acc = c->acc ^ c->RAM[c->instbuffer[1]];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x02://immediate
			c->acc = c->acc ^ c->instbuffer[1];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = c->acc ^ c->RAM[cpos];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		
		case 0x03://ADC
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			cpos = ReadMemory(c,spos);
			ADCFunction(c,cpos);
			c->pc += 2;
			break;
			case 0x01://zeropage
			ADCFunction(c,c->RAM[c->instbuffer[1]]);
			c->pc += 2;
			break;
			case 0x02://immediate
			ADCFunction(c,c->instbuffer[1]);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			ADCFunction(c, ReadMemory(c,spos));
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			ADCFunction(c, ReadMemory(c,spos));
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			ADCFunction(c, c->RAM[cpos]);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			ADCFunction(c, ReadMemory(c,spos));
			c->pc += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			ADCFunction(c, ReadMemory(c,spos));
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		
		case 0x04://STA
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			WriteMemory(c,spos,c->acc);
			c->pc += 2;
			break;
			case 0x01://zeropage
			WriteMemory(c,c->instbuffer[1],c->acc);
			c->pc += 2;
			break;
			case 0x02://immediate
			WriteMemory(c,c->instbuffer[1],c->acc);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			WriteMemory(c,spos,c->acc);
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			WriteMemory(c,spos,c->acc);
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			WriteMemory(c,cpos,c->acc);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			WriteMemory(c,spos,c->acc);
			c->pc += 3;
			break;
			case 0x07://absolute x
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			WriteMemory(c,spos,c->acc);
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		case 0x05://LDA
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x01://zeropage
			c->acc = ReadMemory(c,c->instbuffer[1]);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x02://
			c->acc = ReadMemory(c,c->instbuffer[1]);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = ReadMemory(c,cpos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			case 0x07://absolute x
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		case 0x06://CMP
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 2;
			break;
			case 0x01://zeropage
			temp = c->acc - ReadMemory(c,c->instbuffer[1]);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 2;
			break;
			case 0x02://immediate
			temp = c->acc - c->instbuffer[1];
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			temp = c->acc - ReadMemory(c,cpos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 3;
			break;
			case 0x07://absolute x
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		case 0x07://SBC
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			cpos = ReadMemory(c,spos);
			ADCFunction(c,~cpos);
			c->pc += 2;
			break;
			case 0x01://zeropage
			ADCFunction(c,~c->RAM[c->instbuffer[1]]);
			c->pc += 2;
			break;
			case 0x02://immediate
			ADCFunction(c,~c->instbuffer[1]);
			c->pc += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			ADCFunction(c, ~ReadMemory(c,spos));
			c->pc += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			ADCFunction(c, ~ReadMemory(c,spos));
			c->pc += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			ADCFunction(c, ~c->RAM[cpos]);
			c->pc += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			ADCFunction(c, ~ReadMemory(c,spos));
			c->pc += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			ADCFunction(c, ~ReadMemory(c,spos));
			c->pc += 3;
			break;
			default:
			c->pc++;
			break;
		}
		break;
		default:
		break;
	}
}

void RunSubset2Instructions(struct cpu* c, unsigned char op, unsigned char amode){
	unsigned char cpos = 0;//used for zero page addressing
	unsigned short spos = 0;//used for addressing
	signed char temp = 0; //used for temporary math
	unsigned char utemp = 0;//used for unsigned temporary math
	switch (op){
		case 0x00://ASL
			switch(amode){
				case 0://immediate
					//this shouldnt happen apparently
					c->pc+= 2;
					break;
				case 1://zeropage
					utemp = c->RAM[c->instbuffer[1]];
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					c->RAM[c->instbuffer[1]] = utemp;
					c->pc+= 2;
					break;
				case 2://accumulator
					SetPBit(c,CFLAG,c->acc&0x80);
					c->acc = c->acc<<1;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,c->acc&0x80);
					c->pc+= 1;
					break;
				case 3://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
					break;
				case 4://this case is illegal
					c->pc+= 2;
					break;
				case 5://zero page, x
					cpos = c->instbuffer[1] + c->x;
					utemp = c->RAM[cpos];
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					c->RAM[cpos] = utemp;
					c->pc+= 2;
					break;
				case 6://this case is illegal
					c->pc+= 2;
					break;
				case 7://absolute, x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,temp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
					break;
			}	
			break;
		case 0x01://ROL
			switch(amode){
				case 0://immediate
				//this shouldnt happen apparently
					c->pc+= 2;
					break;
				case 1://zeropage
					utemp = c->RAM[c->instbuffer[1]];
					cpos = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					utemp |= cpos;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					c->RAM[c->instbuffer[1]] = utemp;
					c->pc+= 2;
					break;
				case 2://accumulator
					utemp = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,c->acc&0x80);
					c->acc = c->acc<<1;
					c->acc |= utemp;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,c->acc&0x80);
					c->pc+= 1;
					break;
				case 3://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					utemp = ReadMemory(c,spos);
					cpos = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					utemp |= cpos;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
					break;
				case 4://this case is illegal
					c->pc+= 2;
					break;
				case 5://zero page, x
					cpos = c->instbuffer[1] + c->x;
					utemp = c->RAM[cpos];
					spos = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					utemp |= (spos && 0xFF);
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					c->RAM[cpos] = utemp;
					c->pc+= 2;
					break;
				case 6://this case is illegal
					c->pc+= 2;
					break;
				case 7://absolute, x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					cpos = GetPBit(c,CFLAG);
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,temp&0x80);
					utemp = utemp<<1;
					utemp |= cpos;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
				break;
			}
			break;
		case 0x02://LSR
			switch(amode){
				case 0://immediate
					//this shouldnt happen apparently
					c->pc+= 2;
					break;
				case 1://zeropage
					utemp = c->RAM[c->instbuffer[1]];
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					c->RAM[c->instbuffer[1]] = utemp;
					c->pc+= 2;
					break;
				case 2://accumulator
					SetPBit(c,CFLAG,c->acc&0x01);
					c->acc = c->acc>>1;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,0);
					c->pc+= 1;
					break;
				case 3://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
					break;
				case 4://this case is illegal
					c->pc+= 2;
					break;
				case 5://zero page, x
					cpos = c->instbuffer[1] + c->x;
					utemp = c->RAM[cpos];
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					c->RAM[cpos] = utemp;
					c->pc+= 2;
					break;
				case 6://this case is illegal
					c->pc+= 2;
					break;
				case 7://absolute, x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,temp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
					break;
			}
			break;
		case 0x03://ROR
			switch(amode){
				case 0://immediate
					//this shouldnt happen apparently
					c->pc+= 2;
				break;
				case 1://zeropage
					utemp = c->RAM[c->instbuffer[1]];
					cpos = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					utemp |= cpos << 7;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					c->RAM[c->instbuffer[1]] = utemp;
					c->pc+= 2;
					break;
				case 2://accumulator
					SetPBit(c,CFLAG,c->acc&0x01);
					cpos = GetPBit(c,CFLAG);
					c->acc = c->acc>>1;
					c->acc |= cpos << 7;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,0);
					c->pc+= 1;
					break;
				case 3://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					utemp = ReadMemory(c,spos);
					cpos = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					utemp |= cpos << 7;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
					break;
				case 4://this case is illegal
					c->pc+= 2;
					break;
				case 5://zero page, x
					cpos = c->instbuffer[1] + c->x;
					utemp = c->RAM[cpos];
					spos = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					utemp |= (spos << 7) & 0xFF;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					c->RAM[cpos] = utemp;
					c->pc+= 2;
					break;
				case 6://this case is illegal
					c->pc+= 2;
					break;
				case 7://absolute, x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					utemp = ReadMemory(c,spos);
					cpos = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,temp&0x01);
					utemp = utemp>>1;
					utemp |= cpos << 7;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					WriteMemory(c,spos,utemp);
					c->pc+= 3;
					break;
			}
			break;
		default:
			c->pc++;
			break;
	}
}
void RunInstruction(struct cpu* c){
	FetchInstruction(c);
	unsigned char inst = c->instbuffer[0];//instructions are indexed in the form "aaabbbcc"
	//according to http://nparker.llx.com/a2/opcodes.html
	unsigned char amode = (inst >> 2) & 0b00000111;//addressing mode
	unsigned char op = (inst >> 5) & 0b00000111;//op code
	if (inst & 0x01){//cc == 01
		RunSubset1Instructions(c,op,amode);
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
#endif /* CPU_H_ */