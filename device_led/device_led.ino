
// Arduino code for a SWITCH-enabled LED.
// Communicates over serial connection (in practice, an XBee transmitter).
// Available commands:
//   turnOn: Turns lamp on
//   turnOff: Turns lamp off
//   toggle: Toggles lamp
//   getStatus: Returns status of device in JSON
//   dimX, where X is a number from 1 to 255; 
//

#include <String.h>

#define BUFF_LEN 255
#define DEVICE_ID "Astro"
#define DEVICE_TYPE "LED"

const int ledPin = 9; // the pin that the LED is attached to
int deviceStatus = 0; // deviceStatus of the device
int dimLevel = 255; // dim level of the device

char bufferString[BUFF_LEN+1]; // string to hold the text from the server
int bufferLength = 0;

void setup() {

  // initialize serial communication:
  Serial.begin(9600);

  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);

  // Announce thyself
  Serial.print("{ \"deviceid\" : \"");
  Serial.print(DEVICE_ID);
  Serial.print("\" , \"devicetype\" : \"");
  Serial.print(DEVICE_TYPE);
  Serial.print("\" , \"devicestatus\" : ");
  Serial.print(deviceStatus);
  Serial.println(" }");
}

void loop() {
  // as long as there are bytes in the serial queue, read them
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
  int i;

  for ( i = 0 ; i < BUFF_LEN && bufferString[i] != ',' ; i++) {
  }

  if ( i == BUFF_LEN ) {
    //Serial.println("Error; incorrect syntax, no comma found");
  }

  else if (strncmp(bufferString, DEVICE_ID, i) == 0) {
    i++;
    char* command = bufferString+i;

    //Serial.println("This is for me! I'll do something with it.");
    //Serial.print("Command: ");
    //Serial.println(command);

    // if the command is turnOn, activate the light
    if (strcmp(command, "turnOn") == 0) {
      analogWrite(ledPin, dimLevel);
      deviceStatus = 1;
      //Serial.println("On");
    }

    // if the command is turnOff, deactivate the light
    else if (strcmp(command, "turnOff") == 0) {
      digitalWrite(ledPin, LOW);
      deviceStatus = 0;
      //Serial.println("Off");
    }

    // if the command is "toggle", toggle the light
    else if (strcmp(command, "toggle") == 0) {
      if ( deviceStatus == 1 ) {
        digitalWrite(ledPin, LOW);
        deviceStatus = 0;
      }
      else {
        analogWrite(ledPin, dimLevel);
        deviceStatus = 1;
      }
      //Serial.println("Toggle.");
    }
    
    // 
    else if (strncmp(command, "dim", 3) == 0) {
      char* endptr;
      command = command + 3;
      dimLevel = strtol (command, &endptr, 10);
      if ( dimLevel > 0 && dimLevel < 256 ) {
        analogWrite(ledPin, dimLevel);
      }
      else {
        //Serial.println("Error: Bad dim level");
      }
      //Serial.print("Dim level: ");
      //Serial.println(dimLevel);
    }
      

    // if the command is "getStatus", respond with deviceStatus
    else if (strcmp(command, "getStatus") == 0) {
      Serial.print("{ \"deviceid\" : \"");
      Serial.print(DEVICE_ID);
      Serial.print("\" , \"devicestatus\" : ");
      Serial.print(deviceStatus);
      Serial.println(" }");
    }

    else {
      //Serial.println("Unrecognized command.");
    }
  }
  else {
    //Serial.println("Not for me.");
  }

  // Erase the buffer string
  for (i = 0; i < BUFF_LEN; i++)
    bufferString[i] = '\0';
  bufferLength = 0;
}
