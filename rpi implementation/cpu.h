/*
 * cpu.h
 *
 * Created: 11/18/2019 3:55:33 PM
 *  Author: jrodr087
 */ 


#ifndef CPU_H_
#define CPU_H_
#include "apu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define nbitpos 7
#define vbitpos 6
#define bbitpos 4
#define dbitpos 3
#define ibitpos 2
#define zbitpos 1
#define cbitpos 0
#define stackhead 0x0100
#define mirrorhead 0x1FFF//ram is mirrored so that the address at 0x0001 == 0x0800
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
	uint8_t s; //stack pointer
	uint8_t status; //status register
	/* bit of the status register in order starting from bit 7 down
	N: negative flag set when any arithmetic ops give a negative
	V: overflow flag set on a sign overflow on int8_tacter operations clear otherwsise
	1 unused always 1
	B: break flag always set except when p is pushed on the stack when jumping
	D: decimal mode flag set to select the binary coded decimal mode, typicaly 0
	I: interrupt disable flag set to disable jumping to the IRQ handler vector. automatically set when jumping to the IRQ handler to prevent blocking
	Z:  zero flag set when an arithmetic operation is loaded with the value 0 and clear otherwise
	C: carry flag used in additions and subtractions as a 9th bit
	*/
	int8_t acc; //accumulator
	uint8_t x; //indexing register x
	uint8_t y; // indexing register y
	uint8_t depth; //counts how many routines have been called
	uint8_t playing; //1 if we are currently running the play routine
	uint8_t startingsong;
	uint8_t instbuffer[3]; //buffer for instructions read from spi
	uint8_t RAM[ramsize];//2KB internal ram
	uint8_t cart[0x1FFFF];
	char songname[32];
	char artistname[32];
	char copyright[32];
	uint8_t initbuff[FUNCTIONBUFFSIZE];//internal buffer for the init function
	uint8_t playbuff[FUNCTIONBUFFSIZE];//internal buffer for the play function
	uint16_t progcount; // program counter
	uint16_t playspeed;
	uint16_t playadd;
	uint16_t initadd;
	
	uint16_t clocks;
	enum CPUStatus state;
	struct apu a;
};
void SetPBit(struct cpu* tmp, uint8_t pos, uint8_t val){
	if (val){
		tmp->status |= ( 0x01 << pos);
	}
	else{
		tmp->status &= ~(0x01 << pos);
	}
}
uint8_t GetPBit(struct cpu* c,uint8_t pos){
	return ((c->status >> pos) & 0x01);
}
void PushStack(struct cpu* c, uint8_t val){
	c->s--;
	c->RAM[c->s+stackhead] = val;
}
uint8_t PopStack(struct cpu* c){
	c->s++;
	return c->RAM[c->s-1 + stackhead];
}
uint8_t PeekStack(struct cpu* c){
	return c->RAM[c->s+stackhead];
}

void PopulateBuffers(struct cpu *c){//fills the play and init buffers so we dont have to spi so much
	/*c->progcount = c->initadd;
	for (uint8_t i = 0; i < 255;i++){
		//SPI_Transmit(0xFF);//transmit dummy data so we know we arent ahead of the rpi
		SPI_Transmit(READ);
		SPI_Transmit((c->progcount >>8) & 0xFF);
		SPI_Transmit(c->progcount & 0xFF);
		SPI_Transmit(0xFF);//dummy data so we can get the requested information back on the next cycle
		c->initbuff[i] = SPI_ServantReceive();
		c->progcount+=1;
	}//get the first 255 bytes for the init buffer
	//SPI_Transmit(0xFF);//transmit dummy data so we know we arent ahead of the rpi
	SPI_Transmit(READ);
	SPI_Transmit((c->progcount >>8) & 0xFF);
	SPI_Transmit(c->progcount & 0xFF);
	SPI_Transmit(0xFF);//dummy data so we can get the requested information back on the next cycle
	c->initbuff[255] = SPI_ServantReceive();//gets the last byte for the buffer
	c->progcount = c->playadd;
	for (uint8_t i = 0; i < 255;i++){
		//SPI_Transmit(0xFF);//transmit dummy data so we know we arent ahead of the rpi
		SPI_Transmit(READ);
		SPI_Transmit((c->progcount >>8) & 0xFF);
		SPI_Transmit(c->progcount & 0xFF);
		SPI_Transmit(0xFF);//dummy data so we can get the requested information back on the next cycle
		c->playbuff[i] = SPI_ServantReceive();
		c->progcount+=1;
	}//get the first 255 bytes for the play buffer
	//SPI_Transmit(0xFF);//transmit dummy data so we know we arent ahead of the rpi
	SPI_Transmit(READ);
	SPI_Transmit((c->progcount >>8) & 0xFF);
	SPI_Transmit(c->progcount & 0xFF);
	SPI_Transmit(0xFF);//dummy data so we can get the requested information back on the next cycle
	c->initbuff[255] = SPI_ServantReceive();//gets the last byte*/
	FILE *fileptr;
	uint8_t *buffer;
	long filelen;
	printf("before file read\n");
	fileptr = fopen("smb.nsf", "rb");  // Open the file in binary mode
	fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	filelen = ftell(fileptr);             // Get the current byte offset in the file
	rewind(fileptr);                      // Jump back to the beginning of the file

	buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
	fread(buffer, filelen, 1, fileptr); // Read in the entire file
	fclose(fileptr); // Close the file
	printf("after file read\n");
	if (buffer[0] == 'N' && buffer[1] == 'E' && buffer[2] == 'S' && buffer[3] == 'M' && buffer[4] == 0x1A){
		printf("file read works\n");
	}
	else{printf("file read does not works\n");exit(1);}
	c->startingsong = buffer[7];
	uint16_t loadaddress = buffer[8]+(buffer[9]<<8);
	c->initadd = buffer[0x0A]+(buffer[0x0B]<<8);
	c->playadd = buffer[0x0C]+(buffer[0x0D]<<8);
	for (unsigned char i = 0; i < 32; i++){ 
		c->songname[i] = buffer[0x0e +i];
		c->artistname[i] = buffer[0x2e +i];
		c->copyright[i] = buffer[0x4e +i];
	}
	printf("%d",loadaddress);
	printf("\n");
	printf("%d",c->initadd);
	printf("\n");
	printf("%d",c->playadd);
	printf("\n");
	printf(c->songname);
	printf("\n");
	printf(c->artistname);
	printf("\n");
	printf(c->copyright);
	printf("\n");
	for (unsigned int i = 0; i < filelen - 0x80; i++){
		c->cart[loadaddress+i] = buffer[i+0x80];
		//printf("\n");
	}
	for (unsigned int i = 0; i < 256; i++){
		c->playbuff[i] = c->cart[c->playadd+i];
		c->initbuff[i] = c->cart[c->initadd+i];
	}
	printf("instruction at init: %d\n",c->cart[c->initadd]);
	printf("instruction at play: %d\n",c->cart[c->playadd]);
	printf("instruction at init: %d\n",c->cart[c->initadd]);
	
}

void InitCpu(struct cpu* c){
	c->s = 0xFF;//stack grows downwards
	c->status = 0;
	c->depth = 0;
	c->acc = 0;
	c->playing = 0;
	c->x = 0;
	c->y = 0;
	c->instbuffer[0]=0;
	c->instbuffer[1]=0;
	c->instbuffer[2]=0;
	for (uint16_t i = 0; i < 0x07FF; i++){
		c->RAM[i] = 0;
	}
	for (uint16_t i = 0x6000; i < 0x7FFF; i++){
		c->cart[i] = 0;
	}
	
	PopulateBuffers(c);
	/*c->initadd = SPI_ServantReceive();
	c->initadd += SPI_ServantReceive() << 8;
	c->playadd = SPI_ServantReceive();
	c->playadd += SPI_ServantReceive() << 8;
	c->playspeed = SPI_ServantReceive();
	c->playspeed += SPI_ServantReceive() << 8;
	c->progcount = c->initadd;*/
	c->clocks = 0;
	c->state = running;
}
uint8_t ReadMemory(struct cpu *c, uint16_t pos){
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
	else if (pos == 0x4015){
		return c->a.ce;
	}
	else if (pos >= 0x6000 && pos <= 0xFFF9){
		return c->cart[pos];
	}
}


void WriteMemory(struct cpu *c, uint16_t pos, uint8_t val){//writes to certain memory addresses
	if (pos == 0x07C7){
		printf("val: %02x", val);
		printf("\n");
	}
	if (pos <= mirrorhead){//write to wam
		pos = pos % ramsize;
		c->RAM[pos] = val;
		return;
	}
	if ((pos >= 0x4000 && pos <= 0x4013 ) || pos == 0x4015){//apu write
		APUWrite(&(c->a),val,pos & 0xFF);
	}
	else if (pos >= 0x6000 && pos <= 0x7FFF){
		 c->cart[pos] = val;
	}
	
}
void FetchInstruction(struct cpu* c){
	//	printf("fetch at address: %d",c->progcount);
	//	printf("\n");
	/*	if (c->progcount == 63188){
			printf("hey\n");
		}*/
	for (unsigned int i = 0; i < 3; i++){
		c->instbuffer[i] = c->cart[c->progcount+i];
	//	printf("data: %02X",c->cart[c->progcount+i]);
	//	printf("\n");
	}
}
void ADCFunction(struct cpu* c, uint8_t inc){
	uint16_t sum;
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
void RunSubset1Instructions(struct cpu* c, uint8_t op, uint8_t amode){
	//these instructions typically concern the accumulator
	uint8_t cpos = 0;//used for zero page addressing
	uint16_t spos = 0;//used for addressing
	int8_t temp = 0; //used for temporary math
	switch (op){
		case 0x00://ORA
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x01://zeropage
			c->acc = c->acc | c->RAM[c->instbuffer[1]];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x02://immediate
			c->acc = c->acc | c->instbuffer[1];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = c->acc | c->RAM[cpos];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = c->acc | ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
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
			c->progcount += 2;
			break;
			case 0x01://zeropage
			c->acc = c->acc & c->RAM[c->instbuffer[1]];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x02://immediate
			c->acc = c->acc & c->instbuffer[1];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = c->acc & c->RAM[cpos];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = c->acc & ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
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
			c->progcount += 2;
			break;
			case 0x01://zeropage
			c->acc = c->acc ^ c->RAM[c->instbuffer[1]];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x02://immediate
			c->acc = c->acc ^ c->instbuffer[1];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = c->acc ^ c->RAM[cpos];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = c->acc ^ ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
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
			c->progcount += 2;
			break;
			case 0x01://zeropage
			ADCFunction(c,c->RAM[c->instbuffer[1]]);
			c->progcount += 2;
			break;
			case 0x02://immediate
			ADCFunction(c,c->instbuffer[1]);
			c->progcount += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			ADCFunction(c, ReadMemory(c,spos));
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			ADCFunction(c, ReadMemory(c,spos));
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			ADCFunction(c, c->RAM[cpos]);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			ADCFunction(c, ReadMemory(c,spos));
			c->progcount += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			ADCFunction(c, ReadMemory(c,spos));
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
			break;
		}
		break;
		
		case 0x04://STA
		switch (amode){//addressing mode
			case 0x00://indirect x zeropage
			cpos = c->instbuffer[1] + c->x;
			spos = (c->RAM[cpos+1] << 8) + c->RAM[cpos];
			WriteMemory(c,spos,c->acc);
			c->progcount += 2;
			break;
			case 0x01://zeropage
			WriteMemory(c,c->instbuffer[1],c->acc);
			c->progcount += 2;
			break;
			case 0x02://immediate
				//illegal
				c->progcount += 2;
				break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			WriteMemory(c,spos,c->acc);
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			WriteMemory(c,spos,c->acc);
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			WriteMemory(c,cpos,c->acc);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			WriteMemory(c,spos,c->acc);
			c->progcount += 3;
			break;
			case 0x07://absolute x
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			WriteMemory(c,spos,c->acc);
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
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
			c->progcount += 2;
			break;
			case 0x01://zeropage
			c->acc = ReadMemory(c,c->instbuffer[1]);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x02://immediate
			c->acc = c->instbuffer[1];
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			c->acc = ReadMemory(c,cpos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			case 0x07://absolute x
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			c->acc = ReadMemory(c,spos);
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
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
			c->progcount += 2;
			break;
			case 0x01://zeropage
			temp = c->acc - ReadMemory(c,c->instbuffer[1]);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->progcount += 2;
			break;
			case 0x02://immediate
			temp = c->acc - c->instbuffer[1];
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->progcount += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			temp = c->acc - ReadMemory(c,cpos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->progcount += 3;
			break;
			case 0x07://absolute x
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			temp = c->acc - ReadMemory(c,spos);
			SetPBit(c,NFLAG,temp & 0x80);
			SetPBit(c,ZFLAG, temp == 0x00);
			SetPBit(c,CFLAG, temp >= 0);
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
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
			c->progcount += 2;
			break;
			case 0x01://zeropage
			ADCFunction(c,~c->RAM[c->instbuffer[1]]);
			c->progcount += 2;
			break;
			case 0x02://immediate
			ADCFunction(c,~c->instbuffer[1]);
			c->progcount += 2;
			break;
			case 0x03://absolute
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
			ADCFunction(c, ~ReadMemory(c,spos));
			c->progcount += 3;
			break;
			case 0x04://indirect y
			spos = (ReadMemory(c,c->instbuffer[1]+1) << 8) + ReadMemory(c,c->instbuffer[1])+c->y;
			ADCFunction(c, ~ReadMemory(c,spos));
			c->progcount += 2;
			break;
			case 0x05://zeropage x
			cpos = c->instbuffer[1] + c->x;
			ADCFunction(c, ~c->RAM[cpos]);
			c->progcount += 2;
			break;
			case 0x06://absolute y
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
			ADCFunction(c, ~ReadMemory(c,spos));
			c->progcount += 3;
			break;
			case 0x07:
			spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
			ADCFunction(c, ~ReadMemory(c,spos));
			c->progcount += 3;
			break;
			default:
			c->progcount = c->progcount + 1;
			break;
		}
		break;
		default:
		break;
	}
}

void RunSubset2Instructions(struct cpu* c, uint8_t op, uint8_t amode){
	//these instructions typically concern the x register
	uint8_t cpos = 0;//used for zero page addressing
	uint16_t spos = 0;//used for addressing
	int8_t temp = 0; //used for temporary math
	uint8_t utemp = 0;//used for unsigned temporary math
	switch (op){
		case 0x00://ASL
			switch(amode){
				case 0://immediate
					//this shouldnt happen apparently
					c->progcount+= 2;
					break;
				case 1://zeropage
					utemp = c->RAM[c->instbuffer[1]];
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					c->RAM[c->instbuffer[1]] = utemp;
					c->progcount+= 2;
					break;
				case 2://accumulator
					SetPBit(c,CFLAG,c->acc&0x80);
					c->acc = c->acc<<1;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,c->acc&0x80);
					c->progcount+= 1;
					break;
				case 3://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					WriteMemory(c,spos,utemp);
					c->progcount+= 3;
					break;
				case 4://this case is illegal
					c->progcount+= 2;
					break;
				case 5://zero page, x
					cpos = c->instbuffer[1] + c->x;
					utemp = c->RAM[cpos];
					SetPBit(c,CFLAG,utemp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					c->RAM[cpos] = utemp;
					c->progcount+= 2;
					break;
				case 6://this case is illegal
					c->progcount+= 2;
					break;
				case 7://absolute, x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,temp&0x80);
					utemp = utemp<<1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					WriteMemory(c,spos,utemp);
					c->progcount+= 3;
					break;
			}	
			break;
		case 0x01://ROL
			switch(amode){
				case 0://immediate
				//this shouldnt happen apparently
					c->progcount+= 2;
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
					c->progcount+= 2;
					break;
				case 2://accumulator
					utemp = GetPBit(c,CFLAG);
					SetPBit(c,CFLAG,c->acc&0x80);
					c->acc = c->acc<<1;
					c->acc |= utemp;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,c->acc&0x80);
					c->progcount+= 1;
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
					c->progcount+= 3;
					break;
				case 4://this case is illegal
					c->progcount+= 2;
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
					c->progcount+= 2;
					break;
				case 6://this case is illegal
					c->progcount+= 2;
					break;
				case 7://absolute, x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					cpos = GetPBit(c,CFLAG);
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,temp&0x80);
					utemp = (char)(utemp)<<1;
					utemp |= cpos;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,utemp&0x80);
					WriteMemory(c,spos,utemp);
					c->progcount+= 3;
				break;
			}
			break;
		case 0x02://LSR
			switch(amode){
				case 0://immediate
					//this shouldnt happen apparently
					c->progcount+= 2;
					break;
				case 1://zeropage
					utemp = c->RAM[c->instbuffer[1]];
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					c->RAM[c->instbuffer[1]] = utemp;
					c->progcount+= 2;
					break;
				case 2://accumulator
					SetPBit(c,CFLAG,c->acc&0x01);
					utemp = c->acc;
					utemp = utemp >>1;
					c->acc = utemp;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,0);
					c->progcount+= 1;
					break;
				case 3://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					WriteMemory(c,spos,utemp);
					c->progcount+= 3;
					break;
				case 4://this case is illegal
					c->progcount+= 2;
					break;
				case 5://zero page, x
					cpos = c->instbuffer[1] + c->x;
					utemp = c->RAM[cpos];
					SetPBit(c,CFLAG,utemp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					c->RAM[cpos] = utemp;
					c->progcount+= 2;
					break;
				case 6://this case is illegal
					c->progcount+= 2;
					break;
				case 7://absolute, x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					utemp = ReadMemory(c,spos);
					SetPBit(c,CFLAG,temp&0x01);
					utemp = utemp>>1;
					SetPBit(c,ZFLAG,!utemp);
					SetPBit(c,NFLAG,0);
					WriteMemory(c,spos,utemp);
					c->progcount+= 3;
					break;
			}
			break;
		case 0x03://ROR
			switch(amode){
				case 0://immediate
					//this shouldnt happen apparently
					c->progcount+= 2;
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
					c->progcount+= 2;
					break;
				case 2://accumulator
					SetPBit(c,CFLAG,c->acc&0x01);
					cpos = GetPBit(c,CFLAG);
					c->acc = c->acc>>1;
					c->acc |= cpos << 7;
					SetPBit(c,ZFLAG,!c->acc);
					SetPBit(c,NFLAG,0);
					c->progcount+= 1;
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
					c->progcount+= 3;
					break;
				case 4://this case is illegal
					c->progcount+= 2;
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
					c->progcount+= 2;
					break;
				case 6://this case is illegal
					c->progcount+= 2;
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
					c->progcount+= 3;
					break;
			}
			break;
		case 0x04://STX
			switch (amode){//addressing mode
					case 0x00://indirect x zeropage
					//illegal
					c->progcount+= 2;
					break;
				case 0x01://zeropage
					WriteMemory(c,c->instbuffer[1],c->x);
					c->progcount += 2;
					break;
				case 0x02://immediate
					//illegal
					c->progcount += 2;
					break;
				case 0x03://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					WriteMemory(c,spos,c->x);
					c->progcount += 3;
					break;
				case 0x04://indirect y
					//illegal
					c->progcount += 2;
					break;
				case 0x05://zeropage y
					cpos = c->instbuffer[1] + c->y;
					WriteMemory(c,cpos,c->x);
					c->progcount += 2;
					break;
				case 0x06://absolute y
					//illegal
					c->progcount += 3;
					break;
				case 0x07://absolute x
					//illegal
					c->progcount += 3;
					break;
				default:
					c->progcount = c->progcount + 1;
					break;
			}
			break;
		case 0x05://LDX
			switch (amode){//addressing mode
				case 0x00://immediate
					c->x = c->instbuffer[1];
					SetPBit(c,NFLAG,c->x & 0x80);
					SetPBit(c,ZFLAG, c->x == 0x00);
					c->progcount += 2;
					break;
				case 0x01://zeropage
					c->x = ReadMemory(c,c->instbuffer[1]);
					SetPBit(c,NFLAG,c->x & 0x80);
					SetPBit(c,ZFLAG, c->x == 0x00);
					c->progcount += 2;
					break;
				case 0x02://accumulator
					//illegal
					c->progcount += 2;
					break;
				case 0x03://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					c->x = ReadMemory(c,spos);
					SetPBit(c,NFLAG,c->x & 0x80);
					SetPBit(c,ZFLAG, c->x == 0x00);
					c->progcount += 3;
					break;
				case 0x04://indirect y
					//illegal
					c->progcount += 2;
					break;
				case 0x05://zeropage y
					cpos = c->instbuffer[1] + c->y;
					c->x = ReadMemory(c,cpos);
					SetPBit(c,NFLAG,c->x & 0x80);
					SetPBit(c,ZFLAG, c->x == 0x00);
					c->progcount += 2;
					break;
				case 0x06://absolute y
					//illegal
					c->progcount += 3;
					break;
				case 0x07://absolute y
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->y;
					c->x = ReadMemory(c,spos);
					SetPBit(c,NFLAG,c->x & 0x80);
					SetPBit(c,ZFLAG, c->x == 0x00);
					c->progcount += 3;
					break;
				default:
					c->progcount = c->progcount + 1;
					break;
			}
			break;
		case 0x06://DEC
			switch (amode){//addressing mode
				case 0x00://immediate
					//illegal
					break;
				case 0x01://zeropage
					temp = ReadMemory(c,c->instbuffer[1]);
					temp--;
					WriteMemory(c,c->instbuffer[1],temp);
					SetPBit(c,NFLAG,temp& 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					c->progcount += 2;
				break;
				case 0x02://acc
					//illegal
					c->progcount += 2;
					break;
				case 0x03://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					temp = ReadMemory(c,spos);
					temp--;
					WriteMemory(c,spos,temp);
					SetPBit(c,NFLAG,temp& 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					c->progcount += 3;
					break;
				case 0x04:
					//illegal
					c->progcount += 2;
					break;
				case 0x05://zeropage x
					cpos = c->instbuffer[1] + c->x;
					temp = ReadMemory(c,cpos);
					temp--;
					WriteMemory(c,cpos,temp);
					SetPBit(c,NFLAG,temp& 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					c->progcount += 2;
					break;
				case 0x06://absolute y
					//illegal
					c->progcount += 3;
					break;
				case 0x07://absolute x
					//illegal
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					temp = ReadMemory(c,spos);
					temp--;
					WriteMemory(c,spos,temp);
					SetPBit(c,NFLAG,temp& 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					c->progcount += 3;
					break;
				default:
					c->progcount = c->progcount + 1;
					break;
			}
			break;
		case 0x07://INC
		switch (amode){//addressing mode
			case 0x00://immediate
				//illegal
				break;
			case 0x01://zeropage
				temp = ReadMemory(c,c->instbuffer[1]);
				temp++;
				WriteMemory(c,c->instbuffer[1],temp);
				SetPBit(c,NFLAG,temp& 0x80);
				SetPBit(c,ZFLAG, temp == 0x00);
				c->progcount += 2;
				break;
			case 0x02://acc
				//illegal
				c->progcount += 2;
				break;
			case 0x03://absolute
				spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
				temp = ReadMemory(c,spos);
				temp++;
				WriteMemory(c,spos,temp);
				SetPBit(c,NFLAG,temp& 0x80);
				SetPBit(c,ZFLAG, temp == 0x00);
				c->progcount += 3;
				break;
			case 0x04:
				//illegal
				c->progcount += 2;
				break;
			case 0x05://zeropage x
				cpos = c->instbuffer[1] + c->x;
				temp = ReadMemory(c,cpos);
				temp++;
				WriteMemory(c,cpos,temp);
				SetPBit(c,NFLAG,temp& 0x80);
				SetPBit(c,ZFLAG, temp == 0x00);
				c->progcount += 2;
				break;
			case 0x06://absolute y
				//illegal
				c->progcount += 3;
				break;
			case 0x07://absolute x
				//illegal
				spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
				temp = ReadMemory(c,spos);
				temp++;
				WriteMemory(c,spos,temp);
				SetPBit(c,NFLAG,temp& 0x80);
				SetPBit(c,ZFLAG, temp == 0x00);
				c->progcount += 3;
				break;
			default:
				c->progcount = c->progcount + 1;
				break;
		}
		break;
		default:
			c->progcount = c->progcount + 1;
			break;
	}
}

void RunSubset3Instructions(struct cpu* c, uint8_t op, uint8_t amode){
	//these instructions typically concern the y register
	uint8_t cpos = 0;//used for zero page addressing
	uint16_t spos = 0;//used for addressing
	int8_t temp = 0; //used for temporary math
	uint8_t utemp = 0;//used for unsigned temporary math
	switch (op){
		case 0x01://BIT
			switch(amode){
				case 0://immediate
					c->progcount+= 2;//illegal
					break;
				case 1://zero page
					utemp = ReadMemory(c,c->instbuffer[1]);
					SetPBit(c,VFLAG,utemp & 0x40);
					SetPBit(c,NFLAG,utemp & 0x80);
					utemp = utemp & c->acc;
					SetPBit(c,ZFLAG,utemp);
					c->progcount+= 2;
					break;
				case 3://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					utemp = ReadMemory(c,spos);
					SetPBit(c,VFLAG,utemp & 0x40);
					SetPBit(c,NFLAG,utemp & 0x80);
					utemp = utemp & c->acc;
					SetPBit(c,ZFLAG,utemp);
					c->progcount+=3;
					break;
				case 5://zero page
					c->progcount+= 2;//illegal operation
					break;
				case 7:
					c->progcount+= 3;//illegal
					break;
				default:
					c->progcount = c->progcount + 1;//illegal
					break;
			}
			break;
		case 0x02://JMP
			switch(amode){
				case 0://immediate
					c->progcount+= 2;//illegal
					break;
				case 1://zero page
					//illegal
					c->progcount+= 2;
					break;
				case 3://indirect
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					//spos = ReadMemory(c,spos);
					c->progcount = spos;
					break;
				case 5://zero page
					c->progcount+= 2;//illegal operation
					break;
				case 7:
					c->progcount+= 3;//illegal
					break;
				default:
					c->progcount = c->progcount + 1;//illegal
					break;
			}
			break;
		case 0x03://JMP
			switch(amode){
				case 0://immediate
				c->progcount+= 2;//illegal
				break;
				case 1://zero page
				//illegal
				c->progcount+= 2;
				break;
				case 3://indirect
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					//spos = ReadMemory(c,spos);
					if ((spos & 0xFF) != 0xFF){
						spos = (ReadMemory(c,spos+1)<<8) + ReadMemory(c,spos);
					}
					else{
						spos = (ReadMemory(c,spos) + (ReadMemory(c,spos & 0xFF00) << 8) );//replicates a bug the 6502 has when fetching an indirect address at a page boundary
					}
					c->progcount = spos;
					break;
				case 5://zero page
				c->progcount+= 2;//illegal operation
				break;
				case 7:
				c->progcount+= 3;//illegal
				break;
				default:
				c->progcount = c->progcount + 1;//illegal
				break;
			}
			break;
		case 0x04://STY
			switch (amode){//addressing mode
				case 0x00://immediate
				//illegal
				break;
				case 0x01://zeropage
				WriteMemory(c,c->instbuffer[1],c->y);
				c->progcount += 2;
				break;
				case 0x02://immediate
				//illegal
				c->progcount += 2;
				break;
				case 0x03://absolute
				spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
				WriteMemory(c,spos,c->y);
				c->progcount += 3;
				break;
				case 0x04://indirect y
				//illegal
				c->progcount += 2;
				break;
				case 0x05://zeropage x
					cpos = c->instbuffer[1] + c->x;
					WriteMemory(c,cpos,c->y);
					c->progcount += 2;
					break;
				case 0x06://absolute y
				//illegal
				c->progcount += 3;
				break;
				case 0x07://absolute x
				//illegal
				c->progcount += 3;
				break;
				default:
				c->progcount = c->progcount + 1;
				break;
			}
			break;
		case 0x05://LDY
			switch (amode){//addressing mode
				case 0x00://immediate
					c->y = c->instbuffer[1];
					SetPBit(c,NFLAG,c->y & 0x80);
					SetPBit(c,ZFLAG, c->y == 0x00);
					c->progcount += 2;
					break;
				case 0x01://zeropage
					c->y = ReadMemory(c,c->instbuffer[1]);
					SetPBit(c,NFLAG,c->y & 0x80);
					SetPBit(c,ZFLAG, c->y == 0x00);
					c->progcount += 2;
					break;
				case 0x02://accumulator
					//illegal
					c->progcount += 2;
					break;
				case 0x03://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					c->y = ReadMemory(c,spos);
					SetPBit(c,NFLAG,c->x & 0x80);
					SetPBit(c,ZFLAG, c->x == 0x00);
					c->progcount += 3;
					break;
				case 0x04://indirect y
					//illegal
					c->progcount += 2;
					break;
				case 0x05://zeropage x
					cpos = c->instbuffer[1] + c->x;
					c->y = ReadMemory(c,cpos);
					SetPBit(c,NFLAG,c->y & 0x80);
					SetPBit(c,ZFLAG, c->y == 0x00);
					c->progcount += 2;
					break;
				case 0x06://absolute y
					//illegal
					c->progcount += 3;
					break;
				case 0x07://absolute x
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1] + c->x;
					c->y = ReadMemory(c,spos);
					SetPBit(c,NFLAG,c->y & 0x80);
					SetPBit(c,ZFLAG, c->y == 0x00);
					c->progcount += 3;
					break;
				default:
					c->progcount = c->progcount + 1;
					break;
				}
				break;
		case 0x06://CPY
			switch (amode){//addressing mode
				case 0x00://immediate
					temp = c->y - c->instbuffer[1];
					SetPBit(c,NFLAG,temp & 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					SetPBit(c,CFLAG, temp >= 0);
					c->progcount += 2;
					break;
				case 0x01://zeropage
					temp = c->y - ReadMemory(c,c->instbuffer[1]);
					SetPBit(c,NFLAG,temp & 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					SetPBit(c,CFLAG, temp >= 0);
					c->progcount += 2;
					break;
				case 0x02:
					c->progcount += 2;//illegal
					break;
				case 0x03://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					temp = c->y - ReadMemory(c,spos);
					SetPBit(c,NFLAG,temp & 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					SetPBit(c,CFLAG, temp >= 0);
					c->progcount += 3;
					break;
				case 0x04://indirect y
					//illegal
					c->progcount += 2;
					break;
				case 0x05://zeropage x
					//illegal
					c->progcount += 2;
					break;
				case 0x06://absolute y
					//illegal
					c->progcount += 3;
					break;
				case 0x07://absolute x
					//illegal
					c->progcount += 3;
					break;
				default:
					c->progcount = c->progcount + 1;
					break;
			}
			break;
		case 0x07://CPX
			switch (amode){//addressing mode
				case 0x00://immediate
					temp = c->x - c->instbuffer[1];
					SetPBit(c,NFLAG,temp & 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					SetPBit(c,CFLAG, temp >= 0);
					c->progcount += 2;
					break;
				case 0x01://zeropage
					temp = c->x - ReadMemory(c,c->instbuffer[1]);
					SetPBit(c,NFLAG,temp & 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					SetPBit(c,CFLAG, temp >= 0);
					c->progcount += 2;
					break;
				case 0x02:
					c->progcount += 2;//illegal
					break;
				case 0x03://absolute
					spos = (c->instbuffer[2] << 8) + c->instbuffer[1];
					temp = c->x - ReadMemory(c,spos);
					SetPBit(c,NFLAG,temp & 0x80);
					SetPBit(c,ZFLAG, temp == 0x00);
					SetPBit(c,CFLAG, temp >= 0);
					c->progcount += 3;
					break;
				case 0x04://indirect y
					//illegal
					c->progcount += 2;
				break;
				case 0x05://zeropage x
					//illegal
					c->progcount += 2;
					break;
				case 0x06://absolute y
					//illegal
					c->progcount += 3;
					break;
				case 0x07://absolute x
					//illegal
					c->progcount += 3;
					break;
				default:
					c->progcount = c->progcount + 1;
					break;
			}
			break;
		default:
			c->progcount = c->progcount + 1;
			break;
	}
}

uint8_t RunConditionalBranches(struct cpu* c, uint8_t opcode){
	//these opcodes dont fit in the 3 c=XX subsets
	//so we return 1 if one of these opcodes match
	uint8_t cpos = 0;//used for zero page addressing
	int16_t spos = 0;//used for relative addressing
	int8_t temp = 0; //used for temporary math
	uint8_t utemp = 0;//used for unsigned temporary math
	switch (opcode){
		case 0x10://BPL
			c->progcount+= 2;//pc is incremented before any relative addressing is done.
			if (!GetPBit(c,NFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		case 0x30: //BMI
			c->progcount+= 2;
			if (GetPBit(c,NFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		case 0x50://BVC
			c->progcount+= 2;
			if (!GetPBit(c,VFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		case 0x70://BVS
			c->progcount+= 2;
			if (GetPBit(c,VFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		case 0x90://BCC
			c->progcount+= 2;
			if (!GetPBit(c,CFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		case 0xB0://BCS
			c->progcount+= 2;
			if (GetPBit(c,CFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		case 0xD0://BNE
			c->progcount+= 2;
			if (!GetPBit(c,ZFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		case 0xF0://BEQ
			c->progcount+= 2;
			if (GetPBit(c,ZFLAG)){
				temp = c->instbuffer[1];
				spos = c->progcount + temp;
				c->progcount = spos;
			}
			return 1;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}

uint8_t RunInterruptBranches(struct cpu* c, uint8_t opcode){
	uint8_t cpos = 0;//used for zero page addressing
	uint16_t spos = 0;//used for relative addressing
	int8_t temp = 0; //used for temporary math
	uint8_t utemp = 0;//used for unsigned temporary math
	switch (opcode){
		case 0x00://BRK
			//this instructons probably shouldnt ever happen
			c->progcount = c->progcount + 1;
			PushStack(c,(c->progcount>>8)&0xFF);
			PushStack(c,c->progcount&0xFF);
			PushStack(c,c->status);
			spos = (ReadMemory(c,0xFFFF) << 8)+ReadMemory(c,0xFFFE);
			c->progcount = spos;
			SetPBit(c,BFLAG,1);
			return 1;
			break;
		case 0x20://JSR abs
			spos = c->progcount + 2;
			PushStack(c,(spos>>8)&0xFF);
			PushStack(c,spos&0xFF);
			//PushStack(c,c->status);
			spos = (c->instbuffer[2]<<8) + c->instbuffer[1];
			c->progcount = spos;
			c->depth++;
			return 1;
			break;
		case 0x40://rti
			//this also probably shouldnt happen
			c->status = PopStack(c);
			spos = PopStack(c);
			spos += PopStack(c) << 8;
			c->progcount = spos;
			return 1;
			break;
		case 0x60://RTS
			if (c->depth){//we've run another subroutine from play/init
				spos = PopStack(c);
				spos += PopStack(c) << 8;
				spos++;
				c->progcount = spos;
				c->depth--;
				return 1;
			}
			else{//if we are returning from play/init
				c->playing = 0;
				return 1;
			}
			break;
		default:
			return 0;
			break;
		
	}
}

uint8_t RunSingleByteInstructions(struct cpu* c, uint8_t opcode){
	uint8_t cpos = 0;//used for zero page addressing
	uint16_t spos = 0;//used for relative addressing
	int8_t temp = 0; //used for temporary math
	uint8_t utemp = 0;//used for unsigned temporary math
	switch (opcode){
		case 0x08://PHP
			PushStack(c,c->status);
			c->progcount = c->progcount + 1;
			return 1;
			break;
		case 0x28://PHP
			c->status = PopStack(c);
			c->progcount = c->progcount + 1;
			return 1;
			break;
		case 0x48://PHA
			PushStack(c,c->acc);
			c->progcount = c->progcount + 1;
			return 1;
			break;
		case 0x68://PLA
			c->acc = PopStack(c);
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			return 1;
			break;
		case 0x88://DEY
			c->y--;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->y & 0x80);
			SetPBit(c,ZFLAG, c->y == 0x00);
			return 1;
			break;
		case 0xA8://TAY
			c->y = c->acc;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->y & 0x80);
			SetPBit(c,ZFLAG, c->y == 0x00);
			return 1;
			break;
		case 0xC8://INY
			c->y++;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->y & 0x80);
			SetPBit(c,ZFLAG, c->y == 0x00);
			return 1;
			break;
		case 0xE8://INX
			c->x++;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->x & 0x80);
			SetPBit(c,ZFLAG, c->x == 0x00);
			return 1;
			break;
		case 0x18://CLC
			c->progcount = c->progcount + 1;
			SetPBit(c,CFLAG,0);
			return 1;
			break;
		case 0x38://SEC
			c->progcount = c->progcount + 1;
			SetPBit(c,CFLAG,1);
			return 1;
			break;
		case 0x58://CLI
			c->progcount = c->progcount + 1;
			SetPBit(c,IFLAG,0);
			return 1;
			break;
		case 0x78://SEI
			c->progcount = c->progcount + 1;
			SetPBit(c,IFLAG,1);
			return 1;
			break;
		case 0x98://TYA
			c->acc = c->y;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			return 1;
			break;
		case 0xB8://CLV
			c->progcount = c->progcount + 1;
			SetPBit(c,VFLAG,0);
			return 1;
			break;
		case 0xD8://CLD
			c->progcount = c->progcount + 1;
			SetPBit(c,DFLAG,0);
			return 1;
			break;
		case 0xF8://SED
			c->progcount = c->progcount + 1;
			SetPBit(c,DFLAG,1);
			return 1;
			break;
		case 0x8A://TXA
			c->acc = c->x;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->acc & 0x80);
			SetPBit(c,ZFLAG, c->acc == 0x00);
			return 1;
			break;
		case 0x9A://TXS
			c->s = c->x;
			c->progcount = c->progcount + 1;
			return 1;
			break;
		case 0xAA://TAX
			c->x = c->acc;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->x & 0x80);
			SetPBit(c,ZFLAG, c->x == 0x00);
			return 1;
			break;
		case 0xBA://TSX
			c->x = c->s;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->x & 0x80);
			SetPBit(c,ZFLAG, c->x == 0x00);
			return 1;
			break;
		case 0xCA://DEX
			c->x--;
			c->progcount = c->progcount + 1;
			SetPBit(c,NFLAG,c->x & 0x80);
			SetPBit(c,ZFLAG, c->x == 0x00);
			return 1;
			break;
		case 0xEA://NOP
			c->progcount = c->progcount + 1;
			return 1;
			break;
		default:
			return 0;
			break;
	}
}

void RunInstruction(struct cpu* c){
	FetchInstruction(c);
	uint8_t inst = c->instbuffer[0];//instructions are indexed in the form "aaabbbcc"
	//according to http://nparker.llx.com/a2/opcodes.html
	uint8_t amode = (inst >> 2) & 0b00000111;//addressing mode
	uint8_t op = (inst >> 5) & 0b00000111;//op code
	if (RunConditionalBranches(c,inst) || RunInterruptBranches(c,inst) || RunSingleByteInstructions(c,inst)){
		return;
	}
	if ((inst & 0x03) == 0x01){//cc == 01
		RunSubset1Instructions(c,op,amode);
	}
	if ((inst & 0x03) == 0x02){//cc == 10
		RunSubset2Instructions(c,op,amode);
	}
	if ((inst & 0x03) == 0x00){//cc == 00
		RunSubset3Instructions(c,op,amode);
	}
}

void TickCpu(struct cpu* c){
	switch (c->state){
		case waitrpi:
			//SPI_ServantReceive();
			//spiFlag = 0;
			c->state = running;
			break;
		case init:
			InitCpu(c);
			c->state = waitrpi;
			break;
		case running:
			RunInstruction(c);
			break;
	}
}
#endif /* CPU_H_ */