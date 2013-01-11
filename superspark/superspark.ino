//
// Arduino code for the Spark Socket Prototype.
// Written by Zach Supalla for Hex Labs, Inc.
// Copyright Hex Labs, Inc. 2012
// All rights reserved.
//
// Works with the SuperSpark prototype (6x relays)
//
// Socket has the following components:
//   ATmega32u4 processor (Pro Micro 3.3v/8Mhz bootloader)
//   Roving Networks RN-171 Wi-Fi module
//   6x solid-state relays (pins 5-9)
//   TRIAC leading edge dimmer circuit
//     (based on the InMojo Digital AC Dimmer)
//   Bias Power 5V DC power supply (x2)
//
// Available commands:
//   component X: toggle component X
//   component X Y: turn component X on/off (1 = on, 0 = off)
//   set XXXXXX: change status of all components at once (XXXXXX is 6-digit binary)
//   turnOn: Turn all components on
//   turnOff: Turn all components off
//   getStatus: Returns status of device in JSON
//
// 12/01/2012

// TODO:
// Check the math on the dimmer timing
// Make a legit Wi-Fi library, or find one.
// Automatic authentication
// Automatic reset (if it can't find a good network)

// Libraries. Need EEPROM for saving status
#include <EEPROM.h>

#define PIN_START      4       // The pin # for the first component
#define COMPONENTS     6       // The total number of components
#define BAUD           9600    // Serial communication speed

#define SSID_LENGTH    32
#define PWORD_LENGTH   32
#define BUFFER_LENGTH  256

#define EEPROM_MIN     0
#define EEPROM_MAX     511

#define STATUS_ADDR    0
#define HASINFO_ADDR   2
#define SSID_ADDR      3
#define PWORD_ADDR     36
#define AUTH_ADDR      69

volatile int c[COMPONENTS];

// Device info
char deviceID[32] = "Henry"; // Unique ID for the device
char deviceType[32] = "Arduino"; // Devicetype: light

// Network stuff
char ssid[SSID_LENGTH] = "";           // TODO: Right number of characters?
char pword[PWORD_LENGTH] = "";          // TODO: Right number of characters?
int auth = 0;
boolean hasInfo = 0;

char server[] = "23.21.169.6";
int port = 1307;
char ntp_server[] = "nist1-la.ustiming.org";

boolean foundServer = 0;

// Buffers
char bufferString[BUFFER_LENGTH]; // char[] to hold the text from the server
int bufPos = 0;
char bufferString2[BUFFER_LENGTH]; // char[] to hold text from Serial, for debugging
int buf2Pos = 0;
char recipient[32];
char command[32];

void setup()
{
  Serial.begin(BAUD);
  Serial1.begin(BAUD);

  for (int i = 0 ; i < COMPONENTS ; i++) {
    pinMode(i+PIN_START, OUTPUT);
    c[i] = 0;
  }

  // Check the EEPROM for information.
  hasInfo = EEPROM.read(HASINFO_ADDR);

  // If there's stored info, save it in memory. Device will automatically connect.
  // If there's no stored info, create an ad hoc server.
  if (hasInfo)
  {
    Serial.println("Found information in EEPROM.");
    for (int i = 0; i < SSID_LENGTH; i++)
    {
      ssid[i] = EEPROM.read(SSID_ADDR + i);
    }
    for (int i = 0; i < PWORD_LENGTH; i++)
    {
      pword[i] = EEPROM.read(PWORD_ADDR + i);
    }
    auth = EEPROM.read(AUTH_ADDR);
  }
  else
  {
    Serial.println("Did not find information in EEPROM.");
    createServer();
  }

  sendStatus();
}

void loop()
{
  if ( Serial1.available() )
  {
    char c = Serial1.read();
    Serial.print(c);

    // if you get a newline, clear the line and process the command:
    if (c == '\n') {
      processBuffer(bufferString);
      clearBuffer(bufferString);
      clearBuffer(recipient);
      clearBuffer(command);
      bufPos = 0;
    }
    // add the incoming bytes to the end of line:
    else {
      bufferString[bufPos] = c;
      bufPos++;
    }
  }
  if ( Serial.available() ) {
    char c = Serial.read();
    Serial1.print(c);

    if (c == '\n') {
      processBuffer(bufferString2);
      clearBuffer(bufferString2);
      clearBuffer(recipient);
      clearBuffer(command);
      buf2Pos = 0;
    }
    else {
      bufferString2[buf2Pos] = c;
      buf2Pos++;
    }
  }
}


/***********************
 * HELPER FUNCTIONS
 ***********************/

// Find a character in an array, and return its position.
int findChar(char c[], char f) {
  int len = strlen(c);
  for (int i = 0; i < len; i++) {
    if(c[i] == f) {
      return i;
    }
  }
  return -1;
}

// Copy characters from one string to another, starting at position X and ending at position Y
void copy(char c[], char f[], int x, int y) {
  // Make sure this isn't going to mess up any memory
  if (strlen(c) > y - x || strlen(f) > y - x || y - x < 0 ) {
    Serial.println("ERROR: Can't copy those strings.");
    return;
  }

  // Copy characters one by one
  for (int i = x+1; i < y-x; i++) {
    c[i] = f[x+i];
  }
}


/***********************
 * ACTION FUNCTIONS
 ***********************/
 
void component (byte comp, byte state) {
  Serial.println(comp, BIN);
  Serial.println(state, BIN);
  if (comp >= COMPONENTS) {
    Serial.println("ERROR: There aren't that many components");
    return;
  }
  
  if (state == 1) {
    digitalWrite(comp + PIN_START, HIGH);
    c[comp] = 1;
  } else {
    digitalWrite(comp + PIN_START, LOW);
    c[comp] = 0;
  }
}

void set (char* state) {
  for (int i = 0; i < COMPONENTS; i++) {
    if (state[i] == '1') {
      digitalWrite(i + PIN_START, HIGH);
    } else {
      digitalWrite(i + PIN_START, LOW);
    }
    c[i] = state[i];
  }
}

void cycle (int rounds) {
  for (int i = 0; i < rounds; i++) {
    set("000000");
    delay(100);
    set("100000");
    delay(100);
    set("010000");
    delay(100);
    set("001000");
    delay(100);
    set("000100");
    delay(100);
    set("000010");
    delay(100);
    set("000001");
    delay(100);
  }
}


/***********************
 * MESSAGE PARSING
 ***********************/

void processBuffer(char *message) {

  // Find the syntax-defining characters
  int beginner = findChar(message, '{');
  int separator = findChar(message, ':');
  int ender = findChar(message, '}');

  Serial.println(beginner);
  Serial.println(separator);
  Serial.println(ender);

  // If the syntax-defining characters are not there, return an error
  if (beginner == -1 || separator == -1 || ender == -1 || ender - separator < 1 || separator - beginner < 1) {
    Serial.println("Error; incorrect syntax");
    return;
  }

  char *recipientPtr = message + beginner + 1;
  int recipientChars = separator - beginner - 1;
  strncpy(recipient, recipientPtr, recipientChars);

  char *commandPtr = message + separator + 1;
  int commandChars = ender - separator - 1;
  strncpy(command, commandPtr, commandChars);
  command[commandChars] = '\0';

  Serial.println(recipient);
  Serial.println(command);

  if (strcmp(recipient, deviceID) != 0) {
    Serial.println("ERROR: Not for me.");
    return;
  }
  
  else if (strncmp(command, "component ", 10) == 0) {
    char *param = strtok(command + 10, " ");
    byte comp = (byte) strtol(param, (char **) NULL, 10);
    param = strtok(NULL, "\0");
    byte state = (byte) strtol(param, (char **) NULL, 10);
    Serial.print("Component ");
    Serial.print(comp);
    Serial.print(": Set to ");
    Serial.println(state);
    component(comp, state);
  }
  
  else if (strncmp(command, "set ", 4) == 0) {
    char *param = strtok(command + 4, " ");
    Serial.print("Set components to ");
    Serial.println(param);
    set(param);
  }
    

  else if (strcmp(command, "turnOn") == 0) {
    Serial.println("On");
    char *state = "111111";
    set(state);
  }

  else if (strcmp(command, "turnOff") == 0) {
    Serial.println("Off");
    char *state = "000000";
    set(state);
  }

  else if (strcmp(command, "cycle") == 0) {
    Serial.println("Cycle");
    cycle(5);
  }

  else if (strcmp(command, "getStatus") == 0) {
    Serial.println("Returning status.");
    sendStatus();
  }

  else if (strncmp(command, "ssid", 4) == 0 && strlen(command) <= SSID_LENGTH + 4) {
    strcpy(ssid, command+4); // TODO: Ensure that the string isn't longer than the max length for SSIDs
    Serial.print("Changed SSID to ");
    Serial.println(ssid);
    fixSSID();
  }

  else if (strncmp(command, "pword", 5) == 0 && strlen(command) <= PWORD_LENGTH + 5) {
    strcpy(pword, command+5); // TODO: Ensure that the string isn't longer than the max length for passwords
                              // TODO: Allow for WEP keys too
    Serial.print("Changed password to ");
    Serial.println(pword);
  }

  else if (strncmp(command, "auth", 4) == 0 && strlen(command) == 5) {
    auth = command[4] - '0';
    Serial.print("Changed auth to ");
    Serial.println(auth);
  }

  else if (strcmp(command, "save") == 0) {
    clearEEPROM();
    chartoEEPROM(ssid, SSID_ADDR, SSID_LENGTH);
    chartoEEPROM(pword, PWORD_ADDR, PWORD_LENGTH);
    EEPROM.write(AUTH_ADDR, auth);
    EEPROM.write(HASINFO_ADDR, 1);
    Serial.println("Saved credentials.");
  }

  else if (strcmp(command, "connect") == 0) {
    Serial.println("Connect.");
    connectToServer();
  }

  else if (strcmp(command, "clear") == 0) {
    Serial.println("Clear EEPROM.");
    clearEEPROM();
  }

  else {
    Serial.println("ERROR: Bad message");
  }
}

/***********************
 * WLAN FUNCTIONS
 ***********************/

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
  Serial1.println("set sys autoconn 10");
  delay(500);
  Serial1.println("open");
  delay(500);
  Serial1.println("save");
  delay(500);
  Serial1.println("reboot");
  Serial.println("Rebooting.");
  delay(5000);
  sendStatus();
}

void sendStatus() {
  Serial1.println();
  Serial1.print("{ \"deviceid\" : \"");
  Serial1.print(deviceID);
  Serial1.print("\" , \"devicestatus\" : \"");
  for (int i = 0; i < COMPONENTS; i++) {
    Serial1.print(c[i]);
  }
  Serial1.print("\" , \"devicetype\" : \"");
  Serial1.print(deviceType);
  Serial1.print("\" , \"ssid\" : \"");
  Serial1.print(ssid);
  Serial1.print("\" , \"pword\" : \"");
  Serial1.print(pword);
  Serial1.print("\" , \"auth\" : \"");
  Serial1.print(auth);
  Serial1.println("\" }");
}

void fixSSID() {
  for (int i = 0; i < SSID_LENGTH; i++) {
    if (ssid[i] == ' ') {
      ssid[i] = '$';
    }
  }
}

/***********************
 * EEPROM FUNCTIONS
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

/***********************
 * HELPER FUNCTIONS
 ***********************/

void clearBuffer(char *message) {
  int len = strlen(message);
  for (int i = 0; i < len; i++) {
    message[i] = '\0';
  }
}
