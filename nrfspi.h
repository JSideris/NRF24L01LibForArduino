/*
Copyright Joshua Sideris 2016
josh.sideris@gmail.com
https://github.com/JSideris/NRF24L01LibForArduino
Code is free to use for hobbiest/educational projects but give me a heads up if this is helpful. Please contact me for commercial use.
Retain this copynight notice in all files.
*/

//Include Guard
#ifndef nrf24l01spiinterfaceCLASS
#define nrf24l01spiinterfaceCLASS

//NRF Register Addresses:
#define CONFIG 			0x00//Config Register Flags (RW)
{
	#define PRIM_RX		0x01 //1: PRX, 0: PTX
	#define PWR_UP 		0x02 //1: Power Up, 0: Power Down
	#define CRCO 		0x04 //CRC Encoding scheme. 0: 1 byte, 1: 2 bytes
	#define EN_CRC 		0x08 //Enable CRC. Forced high if EN_AA is not 0.
	#define MASK_MAX_RT	0x10 //Mask MAX_RT interrupt
	#define MASK_TX_DS	0x20 //Mask TX_DS interrupt
	#define MASK_RX_DR	0x40 //Mask RX_DR interrupt
}
#define EN_AA			0x01 
#define EN_RXADDR		0x02 
#define SETUP_AW		0x03 
#define SETUP_RETR		0x04 
#define RF_CH			0x05 
#define RF_SETUP		0x06
{
	//#define LNA_HCURR 0x01 //OBSOLETE
	#define RF_PWR_0	0x02 
	#define RF_PWR_1	0x04
	#define RF_DR_HIGH	0x08 
	#define PLL_LOCK	0x10 
	#define RF_DR_LOW	0x20 
	#define CONT_WAVE	0x80 
}
#define STATUS			0x07
{
	//Status Register Flags
	#define TX_FULL		0x01 //TX FIFO full flag.
	#define RX_P_NO_0	0x02 //Flags 3:1 (00001110) - data pipe number for the RX-FIFO payload. 111 = empty.
	#define RX_P_NO_1	0x04
	#define RX_P_NO_2	0x08
	#define MAX_RT  	0x10 //Max #of TX retrans interrupt.
	#define TX_DS   	0x20 //TX data sent interrupt.
	#define RX_DR   	0x40 //RX data received interrupt.
}
#define OBSERVE_TX		0x08
#define CARRIER_DETECT	0x09
#define RX_ADDR_P0		0x0A
#define RX_ADDR_P1		0x0B
#define RX_ADDR_P2		0x0C
#define RX_ADDR_P3		0x0D
#define RX_ADDR_P4		0x0E
#define RX_ADDR_P5		0x0F
#define TX_ADDR			0x10
#define RX_PW_P0		0x11
#define RX_PW_P1		0x12
#define RX_PW_P2		0x13
#define RX_PW_P3		0x14
#define RX_PW_P4		0x15
#define RX_PW_P5		0x16
#define FIFO_STATUS		0x17

//Commands:
#define READ_REG		0x00 // Define read command to register
#define WRITE_REG		0x20 // Define write command to register
#define RD_RX_PLOAD		0x61 // Define RX payload register address
#define WR_TX_PLOAD		0xA0 // Define TX payload register address
#define FLUSH_TX 		0xE1 // Flushes the TX register.
#define FLUSH_RX 		0xE2 // Flushes the RX register.
#define REUSE_TX_PL		0xE3 // Define reuse TX payload register command
#define NOP				0xFF // Define No Operation, might be used to read status register
#define WR_TX_NO_ACK	0x58

#include <Arduino.h>

class NrfSpiClass{
public:
	static void begin(int sspin, int cepin, bool primRx, byte channel, char *address);
	static void clearFlush();
	static void defaults();
	static bool rxReady();
	static byte sendByteInstruction(byte instruction);
	static void setChannel(byte channel);
	static void setConfig(byte config);
	static void setRxMode();
	static void setTxMode();
	static void setPowerUpReg(bool pwr);
	static void setCarrierOutputMode();
	static void setTxAddress(char *txAddress, byte length);
	static void setRxAddress(char *txAddress, byte length);
	static byte readNext();
	static void txByte(byte data, bool ack);
	static void txData(byte *data, byte numbBytes, bool ack);
	static void writeRegister(unsigned char address, unsigned char value);
	static unsigned int readRegister(byte thisRegister, int bytesToRead);
	static bool carrierDetect();
	static byte getLostPackets();
	static byte getRetransmittedPackets();
	
	static void setChipSelectPin(int pin){chipSelectPin = pin;};
	static int getChipSelectPin(){return chipSelectPin;};
	static void setChipEnablePin(int pin){chipEnablePin = pin;};
	static int getChipEnablePin(){return chipEnablePin;};
	
	static byte configReg;
	static byte setupRFReg;
	
private:
	static void csLo();
	static void csHi();
	static void ceLo();
	static void ceHi();
	static int chipSelectPin;
	static int chipEnablePin;
};



extern NrfSpiClass NrfSpi;

#endif