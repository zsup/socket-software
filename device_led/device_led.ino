
// Arduino code for a SWITCH-enabled LED.
// Communicates over serial connection (in practice, an XBee transmitter).
// Available commands: 0-100
// 0 = off
// 1-99 = various dim levels
// 100 = on
//

#include <String.h>

#define BUFF_LEN 80
#define DEVICE_ID "Elroy"
#define DEVICE_TYPE "LED"

const int ledPin = 13; // the pin that the LED is attached to
int status = 0; // status of the device

char bufferString[BUFF_LEN+1]; // string to hold the text from the server
int bufferLength = 0;

void setup() {
  
  // initialize serial communication:
  Serial.begin(9600);
  
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // as long as there are bytes in the serial queue, 
  // read them and send them out the socket
  while (Serial.available() > 0) {
    char c = Serial.read();

    // if you get a newline, clear the line and process the command:
    if (c == '\n') {
      process();
    }
    // add the incoming bytes to the end of line:
    else {
      bufferString[bufferLength] = c;
      bufferLength++;
    }
  }
}

void process() {
  
  // if the current line ends with turnOn, activate the light
  if (strcmp(bufferString, "turnOn") == 0) {
    digitalWrite(ledPin, HIGH);
    status = 1;
    Serial.println("On");
  }
    
  // if the current line ends with turnOff, deactivate the light
  else if (strcmp(bufferString, "turnOff") == 0) {
    digitalWrite(ledPin, LOW);
    status = 0;
    Serial.println("Off");
  }
  
  // if the current line ends with "toggle", toggle the light
  else if (strcmp(bufferString, "toggle") == 0) {
    if ( status == 1 ) {
      digitalWrite(ledPin, LOW);
      status = 0;
    }
    else {
      digitalWrite(ledPin, HIGH);
      status = 1;
    }
    Serial.println("Toggle.");
  }
  
  else {
    Serial.println("Unrecognized command.");
  }
  
  // Erase the buffer string
  int i;
  for (i = 0; i < BUFF_LEN; i++)
    bufferString[i] = '\0';
  bufferLength = 0;
}
