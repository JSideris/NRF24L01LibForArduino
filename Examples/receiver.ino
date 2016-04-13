#include <SPI.h>
#include "nrfspi.h"
#include "pitches.h"

#define _lf 9
#define _lr 6
#define _rf 5
#define _rr 3
#define _w1 0
#define _w2 1
//TODO: double check the weapon pin.

//Nothing may start the motors unliss this is true.
bool run = true;


// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4,4,4,4,4 };

void setup(){
	//Serial.begin(9600);
	pinMode(2, INPUT);
	pinMode(_lf, OUTPUT);
	pinMode(_lr, OUTPUT);

	NrfSpi.begin(8, 7, true, 100, "Candy");


  /*
  MUSIC
  */
    // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second 
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(_w2, melody[thisNote],noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(_w2);
  }
}

void loop(){
	if(digitalRead(2) == LOW){
		//Serial.print("Interrupt - Status = ");
		byte status = NrfSpi.readRegister(STATUS, 1);
		//Serial.println(status);
		if(status & RX_DR){
			int port = (status & RX_P_NO) >> 1;
			//Serial.print("Data Ready - Port = ");
			//Serial.println(port);
			//We only care about port 0, so if something comes in on a different port we're in trouble.
			if(port == 0){
				//Check the number of available bytes. Should be 2.
				int size = NrfSpi.readRegister(RX_PW_P0, 1);
				//Serial.print("Size of port 0: ");
				//Serial.println(size);
				if(size >= 2){
					//Serial.println(NrfSpi.readNext());
					//Serial.println(NrfSpi.readNext());
					//First reset the command counter, since we've received a valid communication.
					//TODO
					//Now read the next byte and arg and do the work.
					byte command = NrfSpi.readNext();
					byte arg = NrfSpi.readNext();
					doCommand(command, arg);
				}
			}
		}
		else{
			//Uhoh...
			//Serial.println("Some Type of Error.");
			NrfSpi.clearFlush();
		}

		NrfSpi.writeRegister(STATUS, 0xFF);
    
	}
}

void doCommand(byte command, byte arg){
	switch(command){
		case 0x00:
			//Hearbeat
			//TODO
		case 0x01:
			//Power Off
			left(true, 0);
			right(true, 0);
			run  = false;
			break;
		case 0x02:
			//Power on
			run = true;
			break;
		case 0x03:
			//Left Forward
			left(true, arg);
			break;
		case 0x04:
			//Left Backward
			left(false, arg);
			break;
		case 0x05:
			//Left Stop
			left(true, 0);
			break;
		case 0x07:
			//Right Forward
			right(true, arg);
			break;
		case 0x08:
			//Right Backward
			right(false, arg);
			break;
		case 0x09:
			//Right Stop
			right(true, 0);
			break;
		case 0x0B:
			//Right Stop
			weapon(true);
			break;
		case 0x0C:
			//Right Stop
			weapon(false);
			break;
	}
}

void left(bool forward, byte speed){
	if(run){
		if(forward){
			analogWrite(_lr, 0);
			analogWrite(_lf, speed);
		}
		else{
			analogWrite(_lf, 0);
			analogWrite(_lr, speed);
		}
	}
}

void right(bool forward, byte speed){
	if(run){
		if(forward){
			analogWrite(_rr, 0);
			analogWrite(_rf, speed);
		}
		else{
			analogWrite(_rf, 0);
			analogWrite(_rr, speed);
		}
	}
}

void weapon(bool on){
	//TODO
}
