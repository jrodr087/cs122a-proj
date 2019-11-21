/*
 * spi.h
 *
 * Created: 11/18/2019 4:45:45 PM
 *  Author: jrodr087
 */ 


#ifndef SPI_H_
#define SPI_H_

 #include <avr/io.h>
#define DD_SS DDB4
#define DDR_SPI DDRB
#define DD_MOSI DDB5
#define DD_MISO DDB6
#define DD_SCK DDB7
#define SETSSOFF ((PORTB) &= ~(1 << (4)))
#define SETSSON ((PORTB) |= (1 << (4)))
volatile unsigned char spiFlag = 0;
void (*spi_func)() = 0;//possibly pass a pointer to be run from the interrupt
void SPI_MasterInit(void){
	DDR_SPI = (1 << DD_MOSI) | (1<<DD_SCK) | (1<<DD_SS);
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);
}
void SPI_Transmit(unsigned char cData){
	SPDR = cData;
	while (!(SPSR & (1<<SPIF)));
}
void SPI_ServantInit(void){
	sei();
	DDR_SPI = (1<<DD_MISO);
	SPCR = (1<<SPE);// | (1<<SPIE); using polling spi
}
unsigned char SPI_ServantReceive(void)
{
	while (!(SPSR & (1<<SPIF)));
	return SPDR;
}

ISR(SPI_STC_vect){
	spiFlag = 1;
	if (spi_func != 0){
		spi_func();
	}
}



#endif /* SPI_H_ */