
// Arduino code for a SWITCH-enabled dimmable lamp.
// Communicates over serial connection (in practice, an XBee transmitter).
//
// Available commands:
//   turnOn: Turns lamp on
//   turnOff: Turns lamp off
//   toggle: Toggles lamp
//   dimX: Dim the light to a certain level between 0 and 255
//   getdeviceStatus: Returns deviceStatus of device in JSON
//

#include <TimerOne.h>  // From http://www.arduino.cc/playground/Code/Timer1

const int AC_LOAD = 3; // the pin that the TRIAC control is attached to
const int INTERRUPT = 1; // the pin that the zero-cross is connected to
const int BAUD = 9600; // Serial communication speed
boolean deviceStatus = 1; // Status of the device upon initialization. It starts off.
int dimLevel = 255; // Dim level upon initialization.

String DEVICE_ID = "Elroy";
String DEVICE_TYPE = "LED"; // TODO: Should be "Dimmable Lamp". Will have to fix this once the server's updated

String bufferString; // string to hold the text from the server

volatile int i = 0; // Our counter
volatile boolean zc = 0; // Boolean to let us know whether we have crossed zero
volatile boolean triac = 0; // Boolean to store whether triac has been triggered
int freq = 31; // Delay for the frequency of power per step (using 256 steps)

void setup() {

  // initialize serial communication:
  Serial.begin(BAUD);

  // initialize the LED pin as an output:
  pinMode(AC_LOAD, OUTPUT);
  
  // Attach interrupts
  attachInterrupt(INTERRUPT, zero_cross, RISING);
  Timer1.initialize(freq);
  Timer1.attachInterrupt(dim_check, freq);
  
  // Turn on the light
  digitalWrite(AC_LOAD, HIGH);
}

void zero_cross() {
  zc = 1;
  Serial.print(2);
}

void dim_check() {
  if ( zc == 1 ) {
    if ( deviceStatus == 1) {
      if (i >= 255 - dimLevel ) {
        if ( triac == 1 ) {
          digitalWrite(AC_LOAD, LOW);
          triac = 0;
          i = 0;
          zc = 0;
          Serial.print(0);
        } else {
          digitalWrite(AC_LOAD, HIGH);
          triac = 1;
          Serial.print(1);
        }
      } else {
        i++;
      }
    } else {
      if (i >= 255 - dimLevel ) {
        i = 0;
        zc = 0;
      } else {
        i++;
      }
    }
  }
}

void fade(int level, int time) {
  if (dimLevel > level) {
    dimLevel--;
    delay(time);
    fade(level, time);
  }
  else if (dimLevel < level) {
    dimLevel++;
    delay(time);
    fade(level, time);
  }
}

void loop() {
  program2();
}

void program1() {
  // PROGRAM 1: Blink the light
  deviceStatus = 0;
  delay(1000);               // wait for a second
  deviceStatus = 1;    // turn the LED off by making the voltage LOW
  delay(1000);               // wait for a second
}

void program2() {
  // PROGRAM 2: Dimming
  fade(50,5);
  delay(1000);
  fade(100,5);
  delay(1000);
  fade(150,5);
  delay(1000);
  fade(200,5);
  delay(1000);
  fade(250,5);
  delay(1000);
}
