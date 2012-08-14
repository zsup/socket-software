//
// Arduino code for the SWITCH Socket Prototype #2.
// Written by Zach Supalla for Hex Labs, Inc.
// Copyright Hex Labs, Inc. 2012
// All rights reserved.
//
// Socket has the following components:
//   ATmega32u4 processor (Pro Micro 3.3v/8Mhz bootloader)
//   Roving Networks RN-171 Wi-Fi module
//   TRIAC leading edge dimmer circuit
//     (based on the InMojo Digital AC Dimmer)
//   Bias Power 5V DC power supply
//   A multi-color LED for status
//
// Available commands:
//   turnOn: Turns lamp on
//   turnOff: Turns lamp off
//   toggle: Toggles lamp
//   dimX: Dim the light to a certain level between 0 and 255
//   getDeviceStatus: Returns deviceStatus of device in JSON
//
// v0.1
// 08/13/2012

// Libraries. Need a timer for dimming and the WiFly library
#include <TimerOne.h>  // From http://www.arduino.cc/playground/Code/Timer1
#include <WiFlySerial3.h> // My own modified version that uses Serial1
#include <avr/pgmspace.h>

// Dimmer circuit stuff
const int AC_LOAD = 3; // the pin that the TRIAC control is attached to
const int INTERRUPT = 1; // the pin that the zero-cross is connected to
const int BAUD = 9600; // Serial communication speed

prog_int8_t deviceStatus[] PROGMEM = {0}; // Status of the device upon initialization. It starts off.
prog_int8_t dimLevel[] PROGMEM = {255}; // Dim level upon initialization.

volatile int i = 0; // Our counter
volatile boolean zc = 0; // Boolean to let us know whether we have crossed zero
volatile boolean triac = 0; // Boolean to store whether triac has been triggered
int freq = 31; // Delay for the frequency of power per step (using 256 steps)
               // TODO: Refine this number, and make a 50Hz version

// Device info
String deviceID = "Elroy"; // Unique ID for the device
String deviceType = "Socket";

// Network stuff
prog_char ssid[] PROGMEM = ""; // TODO: Right number of characters?
prog_char pword[] PROGMEM = ""; // TODO: Right number of characters?
prog_char auth[] PROGMEM = ""; // TODO: Right number of characters?

char server[] = "23.21.169.6";
int port = 1307;
char ntp_server[] = "nist1-la.ustiming.org";

prog_int8_t hasInfo[] PROGMEM = {0};
volatile boolean foundServer = 0;

String bufferString = ""; // string to hold the text from the server

void setup() {
  // Open the serial gates.
  Serial.begin(9600);
  Serial1.begin(9600);
  
  // initialize the LED pin as an output:
  pinMode(AC_LOAD, OUTPUT);
  
  // Attach interrupts
  attachInterrupt(INTERRUPT, zero_cross, RISING);
  Timer1.initialize(freq);
  Timer1.attachInterrupt(dim_check, freq);
  
  // Turn on the light to its last status.
  if ( pgm_read_byte(deviceStatus) == 1 ) {
    digitalWrite(AC_LOAD, HIGH);
  }
  
  // Wi-Fi setup.
  
  // If we don't have a password and SSID, start a server.
  if ( pgm_read_byte(hasInfo) == 0 ) {
    Serial1.print("$$$");
    delay(500);
    Serial1.print("set wlan ssid SWITCH_");
    Serial1.println(deviceID);
    Serial1.println("set wlan channel 1");
    Serial1.println("set wlan join 4");
    Serial1.println("set ip address 169.254.1.1");
    Serial1.println("set ip netmask 255.255.0.0");
    Serial1.println("set ip dhcp 0");
    Serial1.println("save");
    Serial1.println("reboot");
  }
  
  // If we do have a password and SSID, connect.
  else {
    // Connect to our server.
    Serial1.print("$$$");
    delay(500);
    Serial1.print("set wlan ssid ");
    Serial1.println(ssid);
    Serial1.print("set wlan phrase ");
    Serial1.println(pword);
    Serial1.print("set wlan auth ");
    Serial1.println(auth);
    Serial1.print("set ip address ");
    Serial1.println(server);
    Serial1.print("set ip remote ");
    Serial1.println(port);
    Serial1.println("set sys autoconn 1");
    Serial1.println("open");
    Serial1.println("exit");
  }
}

void loop() {
  // as long as there are bytes in the serial queue, read them
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    Serial.print(c);

    // if you get a newline, clear the line and process the command:
    if (c == '\n') {
      process(bufferString);
      bufferString = "";
    }
    // add the incoming bytes to the end of line:
    else {
      bufferString += c;
    }
  }
}

void zero_cross() {
  zc = 1;
  Serial.println("zc");
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

void process(String message) {
  
  message.trim();
  
  int separator = message.indexOf(',');
  
  if (separator == -1) {
    Serial.println("Error; incorrect syntax, no comma found");
  }
  else {
    String command = message.substring(separator+1);
    String recipient = message.substring(0,separator);
    Serial.print("Recipient: ");
    Serial.println(recipient);
    Serial.print("Command: ");
    Serial.println(command);
    
    if ( recipient == deviceID ) {
      if (command == "turnOn" ) {
        Serial.println("On.");
        deviceStatus = 1;
      }
      else if (command == "turnOff" ) {
        Serial.println("Off.");
        deviceStatus = 0;
      }
      else if (command == "pulse" ) {
        // Pulse
        Serial.println("Pulse.");
        int current = dimLevel;
        fade(0, 2);
        fade(current, 2);
      }
      else if (command == "getStatus" ) {
        Serial.println("Returning status.");
        Serial1.print("{ \"deviceid\" : \"");
        Serial1.print(DEVICE_ID);
        Serial1.print("\" , \"devicestatus\" : ");
        Serial1.print(deviceStatus);
        Serial1.print(" , \"dimval\" : ");
        Serial1.print(dimLevel);
        Serial1.println(" }");
      }
      else if (command.substring(0,3) == "dim" ) {
        command = command.substring(3);
        char buf[command.length()+1];
        command.toCharArray(buf, command.length()+1);
        int req = atoi(buf);
        if ( req >= 0 && req <= 255 ) {
          Serial.print("Dim to ");
          Serial.println(dimLevel);
          fade(req, 5);
        }
        else {
          Serial.println("Error: Not a valid dim level.");
        }
      }
      else if (command.substring(0,4) == "ssid" ) {
        command = command.substring(4);
        ssid = command;  // TODO: Replace space with $
        Serial.print("Set SSID to ");
        Serial.println(ssid);
      }
      else if (command.substring(0,5) == "pword" ) {
        command = command.substring(5);
        pword = command;  // TODO: Allow for WEP keys too
        Serial.print("Set password to ");
        Serial.println(pword);
      }
      else if (command.substring(0,3) == "auth" ) {
        command = command.substring(3);
        sec = command;
        Serial.print("Set security type to ");
        Serial.println(sec);
      }
      else if (command == "connect" ) {
        // Attempt to connect
      }
    }
    else {
      Serial.println("Not for me.");
    }
  }
}
