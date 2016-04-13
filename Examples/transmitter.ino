/*
Copyright Joshua Sideris 2016
josh.sideris@gmail.com
https://github.com/JSideris/NRF24L01LibForArduino
Code is free to use for hobbiest/educational projects but give me a heads up if this is helpful. Please contact me for commercial use.
Retain this copynight notice in all files. 
*/

/*
This code was used in a remote controll to speak with a wireless robot.
*/

#include <SPI.h>
#include "nrfspi.h"

int currentCommand = -1;
int currentParams[2];
bool lastCommandWasParamNext = false;
bool lastCommandWasParamInUpperHexitNext = false;

void setup(){
	Serial.begin(9600);
	Serial.println("helloworld");

	//PIN 2 interrupt.
	//attachInterrupt(0, interruptRequest, FALLING);

	pinMode(2, INPUT);
	pinMode(13, OUTPUT);

	NrfSpi.begin(8, 7, false, 100, "Candy");
	//NrfSpi.setCarrierOutputMode();

	resetCommandsAndParams();
}

void loop(){
	
	//Read Serial Data
	if(Serial.available()){
		byte current = Serial.read();
		if(lastCommandWasParamNext){
			lastCommandWasParamNext = false;
			//Normal param expected.
			if(currentParams[0] == -1){
				currentParams[0] = current;
			}
			else if(currentParams[1] == -1){
				currentParams[1] = current;
			}
			else{
				//BAD - means the computer is sending us weird data.
				resetCommandsAndParams();
			}
		}
		else if(lastCommandWasParamInUpperHexitNext){
			//Shifted param expected.
			lastCommandWasParamInUpperHexitNext = false;
			//Normal param expected.
			if(currentParams[0] == -1){
				currentParams[0] = current >> 4;
			}
			else if(currentParams[1] == -1){
				currentParams[1] = current >> 4;
			}
			else{
				//BAD - means the computer is sending us weird data.
				resetCommandsAndParams();
			}
		}
		else{
			//Then this should be a command.
			//So far we've defined commands from 0 to 4.
			if(current <= 5){
				if(current == 0){
					if(currentCommand != -1){
						lastCommandWasParamNext = true;
					}
					else{
						//Very, very bad. Somehow C# became de-synked and we're now getting a param even though a command hasn't been set.
						resetCommandsAndParams();
					}
				}
				else if(current == 1){
					if(currentCommand != -1){
						lastCommandWasParamInUpperHexitNext = true;
					}
					else{
						//Very, very bad. Somehow C# became de-synked and we're now getting a param even though a command hasn't been set.
						resetCommandsAndParams();
					}
				}
				else{
					currentCommand = current;
				}
			}
			else{
				//We got a param without being told we're getting one. Bad.
				resetCommandsAndParams();
			}
		}
	}
	
	byte toSend[2];
	bool canSend = false;
	//Now check to see if we can send.
	switch(currentCommand){
		case -1:
			//Do nothing - command not set.
			break;
		case 2:
			//Power on/off
			if(currentParams[0] != -1){
				if(currentParams[0] == 0){
					toSend[0] = 0x01;
				}else if(currentParams[0] == 1){
					toSend[0] = 0x02;
				}
				toSend[1] = 0x00;
				canSend = true;
			}
			break;
		case 3:
			//LeftMotorSpeed
			if(currentParams[0] != -1 && currentParams[1] != -1){
				if(currentParams[0] == 0 && currentParams[1] == 0){
					//Stop.
					toSend[0] = 0x05;
					toSend[1] = 0x00;
				}
				else if(currentParams[0] != 0 && currentParams[1] != 0){
					//Brake
					toSend[0] = 0x06;
					toSend[1] = 0x00;
				}
				else if(currentParams[0] != 0){
					//Forward.
					toSend[0] = 0x03;
					toSend[1] = (byte)currentParams[0];
				}
				else if(currentParams[1] != 0){
					//Forward.
					toSend[0] = 0x04;
					toSend[1] = (byte)currentParams[1];
				}
				canSend = true;
			}
			break;
		case 4:
			//RightMotorSpeed
			if(currentParams[0] != -1 && currentParams[1] != -1){
				if(currentParams[0] == 0 && currentParams[1] == 0){
					//Stop.
					toSend[0] = 0x09;
					toSend[1] = 0x00;
				}else if(currentParams[0] != 0 && currentParams[1] != 0){
					//Brake
					toSend[0] = 0x0A;
					toSend[1] = 0x00;
				}else if(currentParams[0] != 0){
					//Forward.
					toSend[0] = 0x07;
					toSend[1] = (byte)currentParams[0];
				}else if(currentParams[1] != 0){
					//Forward.
					toSend[0] = 0x08;
					toSend[1] = (byte)currentParams[1];
				}
				canSend = true;
			}
			break;
		case 5: 
			//Weapon go
			if(currentParams[0] != -1){
				if(currentParams[0] == 0){
					toSend[0] = 0x0B;
				}
				else if(currentParams[0] == 1){
					toSend[0] = 0x0C;
				}
				else if(currentParams[0] == 2){
					toSend[0] = 0x0D;
				}
				toSend[1] = 0x00;
				canSend = true;
			}
			break;
	}
	
	if(canSend){
		NrfSpi.txData(toSend, 2, true);
		resetCommandsAndParams();
	}

	//Check for interrupt
	if(digitalRead(2) == LOW){
		//TODO: Advanced checking for different flags might be useful.
		//Serial.print("Interrupt - Status = ");
		Serial.println(NrfSpi.readRegister(STATUS, 1));
		NrfSpi.clearFlush();
	}
}

void resetCommandsAndParams(){
	currentCommand = -1;
	currentParams[0] = -1;
	currentParams[1] = -1;
	lastCommandWasParamNext = false;
	lastCommandWasParamInUpperHexitNext = false;
}
