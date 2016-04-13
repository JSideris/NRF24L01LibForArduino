/*
Copyright Joshua Sideris 2016
josh.sideris@gmail.com
https://github.com/JSideris/NRF24L01LibForArduino
Code is free to use for hobbiest/educational projects but give me a heads up if this is helpful. Please contact me for commercial use.
Retain this copynight notice in all files.
*/


#include <SPI.h>
#include "nrfspi.h"

NrfSpiClass NrfSpi;

//Default config reg is 00001000
byte NrfSpiClass::configReg = 0x08;
byte NrfSpiClass::setupRFReg = 0x0F;

//Pins + defaults
int NrfSpiClass::chipSelectPin = 8;
int NrfSpiClass::chipEnablePin = 7;

void NrfSpiClass::csLo(){
	digitalWrite(getChipSelectPin(), LOW);
}

void NrfSpiClass::csHi(){
	digitalWrite(getChipSelectPin(), HIGH);
}

void NrfSpiClass::ceLo(){
	digitalWrite(getChipEnablePin(), LOW);
}

void NrfSpiClass::ceHi(){
	digitalWrite(getChipEnablePin(), HIGH);
}

/**
Sets up SPI and registers. Powers up module. Puts module in standby-I mode.
primRx is the value of the PRIM_RX register and defines whether the device will be in rx or tx mode.
address is the rx or tx address (depending on primRx).
**/
void NrfSpiClass::begin(int sspin, int cepin, bool primRx, byte channel, char *address){
	//Setup SPI
	SPI.begin();
	//TODO: Experimental clock rate.
	SPI.setClockDivider(SPI_CLOCK_DIV4); //Arduino-defined define.
	SPI.setBitOrder(MSBFIRST); //Arduino-defined define.
	
	setChipEnablePin(cepin);
	pinMode(cepin, OUTPUT); //Arduino-defined define.
	ceLo();
	
	setChipSelectPin(sspin);
	pinMode(sspin, OUTPUT); //Arduino-defined define.
	csHi();
	
	//The module needs 100ms before it can be used.
	//Wait 100ms in case the mcu hasn't been on for that long.
	delay(100);
	
	defaults();
	
	//Set channel
	setChannel(channel);
	
	//1.5 ms delay required to get to standby-I
	delayMicroseconds(1500);
	
	//Actual settings.
	if(primRx){
		//Turn off all pipes accept 1
		//rx address (pipe 0)
		setRxAddress(address, 5);
		setRxMode();
	}
	else{
		//Turn off all pipes
		//tx address
		setTxAddress(address, 5);
		setRxAddress(address, 5);
		setTxMode();
	}
}

/**
Clears status and flushes both rx and tx fifos.
**/
void NrfSpiClass::clearFlush(){
	//Clear status
	NrfSpi.writeRegister(STATUS, 0x7F);
	
	//In case the mcu was reset but the tranceiver was kept on, flush rx/tx.
	sendByteInstruction(FLUSH_TX);
	sendByteInstruction(FLUSH_RX);
}

/**
Resets all the registeres to their defaults.
**/
void NrfSpiClass::defaults(){
	char defaultAddr[] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	clearFlush();
	
	//Set up static payload of 2 bytes.
	writeRegister(RX_PW_P0, 2);
	setTxAddress(defaultAddr, 5);
	setRxAddress(defaultAddr, 5);
	writeRegister(EN_AA, 0x3F); //Default provided by datasheet (all pipes have AA en).
	writeRegister(SETUP_RETR, 0x1F); //0.5ms delay, 15 attempts.
	writeRegister(EN_RXADDR, 0x01); //Only interested in receiving from one pipe.
	setupRFReg = RF_DR_HIGH | RF_PWR_0 | RF_PWR_1; //This should be 0x0E, default provided by datasheet
	writeRegister(RF_CH, 2); //Default provided by datasheet
	writeRegister(RF_SETUP, setupRFReg);
	setConfig(CRCO | EN_CRC | PWR_UP | PRIM_RX);//Default settings for this application, not in datasheet.
	delayMicroseconds(150);
}

bool NrfSpiClass::carrierDetect(){
	return readRegister(CARRIER_DETECT, 1) == 1;
}

byte NrfSpiClass::getLostPackets(){
	return readRegister(OBSERVE_TX, 1) >> 4;
}

byte NrfSpiClass::getRetransmittedPackets(){
	return readRegister(OBSERVE_TX, 1) | 0xF;
}

/**
Sets n-bit tx address. Please ensure to keep length equal for tx and rx.
**/
void NrfSpiClass::setTxAddress(char *txAddress, byte length){
	//First set the address width to length.
	if(length >= 3 && length <= 5){
		writeRegister(SETUP_AW, length - 2);
		
		//Now set the address.
		csLo();
		SPI.transfer(WRITE_REG | TX_ADDR);
		for(byte x = 0; x < length; x++)
			SPI.transfer(txAddress[x]);
		csHi();
	}
}

/**
Sets n-bit rx address. Please ensure to keep length equal for tx and rx.
**/
void NrfSpiClass::setRxAddress(char *rxAddress, byte length){
	//First set the address width to length.
	if(length >= 3 && length <= 5){
		writeRegister(SETUP_AW, length - 2);
		
		//Now set the address.
		csLo();
		SPI.transfer(WRITE_REG | RX_ADDR_P0);
		for(byte x = 0; x < length; x++)
			SPI.transfer(rxAddress[x]);
		csHi();
	}
}

void NrfSpiClass::setChannel(byte channel){
	writeRegister(RF_CH, channel);
}

void NrfSpiClass::setConfig(byte config){
	configReg = config;
	writeRegister(CONFIG, config);
}

/**
Powers up module and puts into rx mode.
**/
void NrfSpiClass::setRxMode(){
	ceLo();
	
	//We want to enable the prim rx bit.
	setConfig(configReg | PRIM_RX);
	ceHi(); 
	delayMicroseconds(150);
}

/**
Sets the last bit of CONFIG to 0, powers up module.
Technically since CE is Lo this will just put the module into standby-I until the TX FIFO is not empty.
But this prepares the module to send data.
**/
void NrfSpiClass::setTxMode(){

	ceLo();

	//We want to disable the PRIM_RX bit.
	//Also need to make sure that pwr up is on, otherwise it's not really tx mode.
	setConfig(configReg & (~PRIM_RX));
	delayMicroseconds(150);
}

/**
This should only be called from standby-I ideally.
**/
void NrfSpiClass::setPowerUpReg(bool pwr){
	//Just turn off the power to go into standby I.
	if(pwr){
		setConfig(configReg | PWR_UP);
		//Takes 1.5ms to power up.
		delayMicroseconds(1500);
	}
	else{
		setConfig(configReg & (~PWR_UP));
	}
}

/**
This broadcasts a carrier signal following the guidelines on P 68 of the nRF24L01 Product Specification @ Appendix C - Carrier wave output power.
Assuming PWR_UP is already 1.
**/
#ifdef DEBUGMODE
void NrfSpiClass::setCarrierOutputMode(){
	char addr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	byte fifo[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	clearFlush();
	setTxMode();
	writeRegister(EN_AA, 0x00);
	writeRegister(SETUP_RETR, 0x00);
	
	setupRFReg = setupRFReg | PLL_LOCK;
	writeRegister(RF_SETUP, setupRFReg);
	setTxAddress(addr, 5);
	setConfig(configReg & (~EN_CRC));
	
	txData(fifo, 32, true);
	delay(1);
	ceHi();
	sendByteInstruction(REUSE_TX_PL);
	
}
#endif

/**
Check whether there is data ready to be received.
**/
bool NrfSpiClass::rxReady(){
	return sendByteInstruction(NOP) & RX_DR != 0;
}

/**
Gets the next byte from rx payload.
**/
byte NrfSpiClass::readNext(){
	ceLo();
	csLo();
	SPI.transfer(RD_RX_PLOAD);
	byte x = SPI.transfer(0x00);
	csHi();
	ceHi();
	return x;
}

/**
Writes a byte to be transmitted.
**/
void NrfSpiClass::txByte(byte data, bool ack){
	//TODO: check that it's in tx mode.
	//TODO: Don't know if this is acceptable. You need to sent two bytes right?

	//Write data to tx fifo.
	csLo();
	if(ack)
		SPI.transfer(WR_TX_PLOAD);
	else
		SPI.transfer(WR_TX_NO_ACK);
	SPI.transfer(data);
	csHi();
	
	//Now need to pulse CE.
	ceHi();
	delayMicroseconds(10); //> 10us pulse is required to initiate transmit mode. 
	ceLo();
	
	//Wait while sending.
	/*while(!(sendByteInstruction(NOP) & (TX_DS | MAX_RT))){
		Serial.println(sendByteInstruction(NOP));
	}*/
}

/**
Writes multiple bytes. Up to 32.
**/
void NrfSpiClass::txData(byte *data, byte numbBytes, bool ack){
	//TODO: At some point I should add a limitation of payload width.
	//Switch to tx mode.
	//Assuming we're already in tx mode.
	//setTxMode();

	//Write data to tx fifo.
	csLo();
	SPI.transfer(WR_TX_PLOAD);
	for(byte x = 0; x < 32 && x < numbBytes; x++){
		SPI.transfer(data[x]);
	}
	csHi();
	
	//Now need to pulse CE.
	ceHi();
	delayMicroseconds(10); //> 10us pulse is required to initiate transmit mode. 
	ceLo();
}

/**
Send an instruction (see api.h). Instruction is sent in one byte with no addational param bytes.
Returns status register.
**/
byte NrfSpiClass::sendByteInstruction(byte instruction){
	csLo();
	byte x = SPI.transfer(instruction);
	csHi();
	return x;
}

void NrfSpiClass::writeRegister(unsigned char address, unsigned char value) {
	csLo();
	//write command is 001xxxxx where xxxxx is the address.
	SPI.transfer(WRITE_REG | address);
	SPI.transfer(value);
	csHi();
}

/**
Read from register.
bytesToRead specifies the number of bytes in the register; either 1 or 2.
**/
unsigned int NrfSpiClass::readRegister(byte address, int bytesToRead) {
	unsigned int inByte = 0;	// incoming byte from the SPI
	unsigned int result = 0;	// result to return
	
	//now combine the address and the command into one byte
	//Read command is just 000xxxxx where xxxxx is the address.
	byte command = READ_REG | address;
	
	// take the chip select Lo to select the device:
	csLo();
	
	// send the device the register you want to read:
	SPI.transfer(command);
	inByte = SPI.transfer(0x00);

	// if you still have another byte to read:
	if (bytesToRead > 1) {
		result = SPI.transfer(0x00) << 8;
		result = result | inByte;
	}
	// take the chip select Hi to de-select:
	csHi();
	// return the result:
	return(result | inByte);
}