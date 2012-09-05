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
// v0.2
// 08/27/2012

// TODO:
// Check the math on the dimmer timing
// Make a legit Wi-Fi library, or find one.
// Automatic authentication
// Automatic reset (if it can't find a good network)

// Libraries. Need a timer for dimming, and EEPROM for saving.
#include <TimerOne.h>  // From http://www.arduino.cc/playground/Code/Timer1
#include <EEPROM.h>

// Dimmer circuit stuff
const int AC_LOAD = 3; // the pin that the TRIAC control is attached to
const int INTERRUPT = 1; // the pin that the zero-cross is connected to
const int BAUD = 9600; // Serial communication speed

volatile int deviceStatus = 0; // Status of the device upon initialization. Defaults to off.
volatile int dimLevel = 255; // Dim level upon initialization. Defaults to max (255).

volatile int i = 0; // Our counter
volatile boolean zc = 0; // Boolean to let us know whether we have crossed zero
volatile boolean triac = 0; // Boolean to store whether triac has been triggered
int freq = 31; // Delay for the frequency of power per step (using 256 steps)
               // TODO: Refine this number, and make a 50Hz version

// Device info
char deviceID[32] = "Elroy"; // Unique ID for the device
char deviceType[32] = "Light"; // Devicetype; light 

// Network stuff
int SSID_LENGTH = 32;
int PWORD_LENGTH = 32;

char ssid[32] = "";           // TODO: Right number of characters?
char pword[32] = "";          // TODO: Right number of characters?
int auth = 0;
boolean hasInfo = 0;

char server[] = "23.21.169.6";
int port = 1307;
char ntp_server[] = "nist1-la.ustiming.org";

boolean foundServer = 0;

// EEPROM stuff
const int EEPROM_MIN = 0;
const int EEPROM_MAX = 511;

const int STATUS_ADDR = 0;
// const int DIMLEVEL_ADDR = 1;
int HASINFO_ADDR = 2;
int SSID_ADDR = 3;
int PWORD_ADDR = 36;
int AUTH_ADDR = 69;

String bufferString = ""; // string to hold the text from the server
                          // TODO: switch to char[256] for clarity
                          
String bufferString2 = "";

void setup() {
  // Open the serial gates.
  Serial.begin(9600);
  Serial1.begin(9600);
  
  // For debugging; wait until Serial is connected to go ahead.
  /*
  for (int i = 0; i < 10; i++) {
    delay(1000);
    Serial.print("Waiting ");
    Serial.println(i);
  }
  */
  
  Serial.println("Serial initialized.");
  
  // initialize the LED pin as an output:
  pinMode(AC_LOAD, OUTPUT);
  
  // Attach interrupts
  attachInterrupt(INTERRUPT, zero_cross, RISING);
  Timer1.initialize(freq);
  Timer1.attachInterrupt(dim_check, freq);
  
  // Check the EEPROM for the last status, and flip it.
  deviceStatus = EEPROM.read(STATUS_ADDR);
  // Turn on the light to its last status.
  if ( deviceStatus == 1 ) {
    digitalWrite(AC_LOAD, HIGH);
    EEPROM.write(STATUS_ADDR, 0);
  } else {
    EEPROM.write(STATUS_ADDR, 1);
  }
  
  // Check the EEPROM for information.
  hasInfo = EEPROM.read(HASINFO_ADDR);
  
  // If there's stored info, attempt to connect; otherwise, create an ad hoc server.
  if (hasInfo) {
    Serial.println("Found information in EEPROM.");
    for (int i = 0; i < SSID_LENGTH; i++) {
      ssid[i] = EEPROM.read(SSID_ADDR + i);
    }
    for (int i = 0; i < PWORD_LENGTH; i++) {
      pword[i] = EEPROM.read(PWORD_ADDR + i);
    }
    auth = EEPROM.read(AUTH_ADDR);
    connectToServer();
  } else {
    Serial.println("Did not find information in EEPROM.");
    createServer();
  }
}

void loop() {
  // as long as there are bytes in the serial queue, read them
  if ( Serial1.available() ) {
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
  if ( Serial.available() ) {
    char c = Serial.read();
    
    if (c == '\n') {
      process(bufferString2);
      bufferString2 = "";
    }
    else {
      bufferString2 += c;
    }
  }
}

void zero_cross() {
  zc = 1;
  i = 0;
  // Serial.println("zc");
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

void dim_check1() {
  if (deviceStatus == 0 || dimLevel < 10) {
    digitalWrite(AC_LOAD, LOW);
  }
  else if (deviceStatus == 1 && dimLevel > 250) {
    digitalWrite(AC_LOAD, HIGH);
  }
  else if (zc == 1) {
    if (i >= 255 - dimLevel ) {
      if (triac == 1) {
        digitalWrite(AC_LOAD, LOW);
        triac = 0;
        i = 0;
        zc = 0;
      } else {
        digitalWrite(AC_LOAD, HIGH);
        triac = 1;
      }
    } else {
      i++;
    }
  }
}

void dim_check2() {
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
      digitalWrite(AC_LOAD, LOW);
      if (i >= 255 - dimLevel ) {
        i = 0;
        zc = 0;
      } else {
        i++;
      }
    }
  }
}

void dim_check4() {
  // When the zero-cross passes, start the counter
  if (zc == 1) {
    if (i >- 255 - dimLevel ) {
      i = 0;
      zc = 0;
    } else {
      i++;
    }
  }
  if (triac == 1) {
    digitalWrite(AC_LOAD, LOW);
    triac = 0;
  }
  if (deviceStatus == 0 || dimLevel < 10) {
    digitalWrite(AC_LOAD, LOW);
  }
  else if (deviceStatus == 1 && dimLevel > 250) {
    digitalWrite(AC_LOAD, HIGH);
  }
  else if (i >= 255 - dimLevel) {
    digitalWrite(AC_LOAD, HIGH);
    triac = 1;
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
        Serial1.print(deviceID);
        Serial1.print("\" , \"devicestatus\" : ");
        Serial1.print(deviceStatus);
        Serial1.print(" , \"dimval\" : ");
        Serial1.print(dimLevel);
        Serial1.print(" , \"ssid\" : \"");
        Serial1.print(ssid);
        Serial1.print("\" , \"pword\" : \"");
        Serial1.print(pword);
        Serial1.print("\" , \"auth\" : \"");
        Serial1.print(auth);
        Serial1.println("\" }");
      }
      else if (command.substring(0,3) == "dim" ) {
        command = command.substring(3);
        char buf[command.length()+1];
        command.toCharArray(buf, command.length()+1);
        int req = atoi(buf);
        if ( req >= 0 && req <= 255 ) {
          Serial.print("Dim to ");
          Serial.println(req);
          fade(req, 5);
        }
        else {
          Serial.println("Error: Not a valid dim level.");
        }
      }
      else if (command.substring(0,4) == "ssid" ) {
        command = command.substring(4);
        char buf[command.length()+1];
        command.toCharArray(buf, command.length()+1);
        strcpy (ssid, buf);  // TODO: Replace space with $
        Serial.print("Set SSID to ");
        Serial.println(ssid);
        fixSSID();
      }
      else if (command.substring(0,5) == "pword" ) {
        command = command.substring(5);
        char buf[command.length()+1];
        command.toCharArray(buf, command.length()+1);
        strcpy (pword, buf);  // TODO: Allow for WEP keys too
        Serial.print("Set password to ");
        Serial.println(pword);
      }
      else if (command.substring(0,4) == "auth" ) {
        command = command.substring(4);
        char buf[command.length()+1];
        command.toCharArray(buf, command.length()+1);
        int req = atoi(buf);
        if ( req >= 0 && req <= 4 ) {
          auth = req;
          Serial.print("Set security type to ");
          Serial.println(auth);
        }
        else {
          Serial.println("Error: Not a valid auth code.");
        }
      }
      else if (command == "save" ) {
        // TODO: Need to make sure this doesn't effect the light status
        clearEEPROM();
        chartoEEPROM(ssid, SSID_ADDR, SSID_LENGTH);
        chartoEEPROM(pword, PWORD_ADDR, PWORD_LENGTH);
        EEPROM.write(AUTH_ADDR, auth);
        EEPROM.write(HASINFO_ADDR, 1);
        Serial.println("Saved authentication.");
      }
      else if (command == "connect" ) {
        Serial.println("Connect.");
        connectToServer();
      }
      else if (command == "clear" ) {
        Serial.println("Clear EEPROM.");
        clearEEPROM();
      }
    }
    else {
      Serial.println("Not for me.");
    }
  }
}

void createServer() {
  Serial.println("Starting server.");
  Serial1.print("$$$");
  delay(1000);
  Serial1.print("set wlan ssid SWITCH_");
  Serial1.println(deviceID);
  delay(500);
  Serial1.println("set wlan channel 1");
  delay(500);
  Serial1.println("set wlan join 4");
  delay(500);
  Serial1.println("set ip address 169.254.1.1");
  delay(500);
  Serial1.println("set ip netmask 255.255.0.0");
  delay(500);
  Serial1.println("set ip dhcp 0");
  delay(500);
  Serial1.println("save");
  delay(500);
  Serial1.println("reboot");
}

void connectToServer() {
  // Connect to our server.
  Serial.println("Attempting to connect");
  Serial1.print("$$$");
  delay(1000);
  Serial1.print("set wlan ssid ");
  Serial1.println(ssid);
  delay(500);
  Serial1.print("set wlan phrase ");
  Serial1.println(pword);
  delay(500);
  Serial1.print("set wlan auth ");
  Serial1.println(auth);
  delay(500);
  Serial1.println("set wlan channel 0");
  delay(500);
  Serial1.println("set wlan join 1");
  delay(500);
  Serial1.println("set ip dhcp 1");
  delay(500);
  Serial1.print("set ip host ");
  Serial1.println(server);
  delay(500);
  Serial1.print("set ip remote ");
  Serial1.println(port);
  delay(500);
  Serial1.println("set sys autoconn 1");
  delay(500);
  Serial1.println("open");
  delay(500);
  Serial1.println("save");
  delay(500);
  Serial1.println("reboot");
  Serial.println("Rebooting.");
  delay(5000);
  instantiate();
}

void instantiate() {
  Serial1.println();
  Serial1.println(" {\"deviceid\": \"Elroy\" }");
}

void fixSSID() {
  for (i = 0; i < SSID_LENGTH; i++) {
    if (ssid[i] == ' ') {
      ssid[i] = '$';
    }
  }
}

/***********************
EEPROM FUNCTIONS
***********************/

void clearEEPROM () {
  for (int i=EEPROM_MIN; i <= EEPROM_MAX; i++) {
    EEPROM.write(i, 0);
  }
}

void readEEPROMtoSerial () {
  for (int i = EEPROM_MIN; i <= EEPROM_MAX; i++) {
    char c = EEPROM.read(i);
    Serial.print(c);
  }
}

void eepromtoChar (int addr, char c[], int len) {
  for (int i = 0; i < len; i++) {
    c[i] = EEPROM.read(addr+i);
  }
}

void chartoEEPROM (char c[], int addr, int len) {
  for (int i = 0; i < len; i++) {
    EEPROM.write(addr+i, c[i]);
  }
}
