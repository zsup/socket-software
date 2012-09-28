//
// Arduino code for the Spark Socket Prototype.
// Written by Zach Supalla for Hex Labs, Inc.
// Copyright Hex Labs, Inc. 2012
// All rights reserved.
//
// Works with Prototype #2 (ugly Wi-Fi)
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
//   fade X Y: Fade to X (0-255) over Y (0-65535) hundredths of a second
//
// Known issues:
//   - There's a long delay during boot-up
//   - After 10 minutes or so, the device starts acting wacky and needs to be rebooted
//   - The whole 'alternate start-up lighting' thing doesn't really work
//
// v0.3
// 09/12/2012

// TODO:
// Check the math on the dimmer timing
// Make a legit Wi-Fi library, or find one.
// Automatic authentication
// Automatic reset (if it can't find a good network)

// Libraries. Need a timer for dimming with PWM, and EEPROM for saving.
#include <EEPROM.h>
#include "TimerOne.h"  // From http://www.arduino.cc/playground/Code/Timer1
#include "Fader.h"

#define TRIAC          3       // the pin that the TRIAC control is attached to
#define INTERRUPT      1       // the pin that the zero-cross is connected to
#define BAUD           9600    // Serial communication speed

#define SSID_LENGTH    32
#define PWORD_LENGTH   32
#define BUFFER_LENGTH  256

#define EEPROM_MIN     0
#define EEPROM_MAX     511

#define DIM_MIN        0
#define DIM_MAX        255

#define STATUS_ADDR    0
#define HASINFO_ADDR   2
#define SSID_ADDR      3
#define PWORD_ADDR     36
#define AUTH_ADDR      69

// slightly less than 1/256th of a half period of the alternating current, in microseconds
// this is 60 Hz, but works for 50 Hz overseas, where the value is 38
#define TIMER_MICROSECONDS  32L

volatile int deviceStatus = 0; // Status of the device upon initialization. Defaults to off.
volatile int dimLevel = 255; // Dim level upon initialization. Defaults to max (255).

volatile int i = 0; // Our counter for zero cross events
volatile boolean intr = 0; // Boolean to let us know whether an interrupt has been activated

// Device info
char deviceID[32] = "Elroy"; // Unique ID for the device
char deviceType[32] = "Light"; // Devicetype; light

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

Fader fader;

void setup()
{
  Serial.begin(BAUD);
  Serial1.begin(BAUD);

  pinMode(TRIAC, OUTPUT);

  Timer1.initialize(TIMER_MICROSECONDS);

  // Check the EEPROM for the last status, and flip it.
  deviceStatus = EEPROM.read(STATUS_ADDR);
  if ( deviceStatus == 1 )
  {
    digitalWrite(TRIAC, HIGH);
    EEPROM.write(STATUS_ADDR, 0);
  }
  else
  {
    EEPROM.write(STATUS_ADDR, 1);
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
 * LIGHTING FUNCTIONS
 ***********************/

// Turn the light to a certain dim level.
// Ensures that we're not using interrupts unless necessary.
// In other words, no interrupts when the light is totally on or off.
void light() {
  // If the dim level is at minimum, turn off the light and interrupts
  if (dimLevel == DIM_MIN || deviceStatus == 0) {
    if (intr) {
      detachInterrupt(INTERRUPT);
      intr = 0;
    }
    digitalWrite(TRIAC, LOW);
  }

  // If the dim level is at maximum, turn on the light and turn off interrupts
  else if (dimLevel > DIM_MAX - 5) {
    if (intr) {
      detachInterrupt(INTERRUPT);
      intr = 0;
    }
    digitalWrite(TRIAC, HIGH);
  }

  // Otherwise, attach the interrupt and let zero_cross() do the rest
  else {
    attachInterrupt(INTERRUPT, zero_cross, RISING);
    intr = 1;
  }
}

// Function called when the zero-cross happens.
// Resets the counter and activates the dimmer counter.
void zero_cross() {
  i = 0;
  Timer1.attachInterrupt(dim_check);

  if (fader.is_fading()) {
    dimLevel = fader.current_level(millis());
  }
}

void dim_check() {
  // First, increment.
  i++;

  // Check to see if the counter has reached the right point. If it has, trip the TRIAC.
  if ( i >= DIM_MAX - dimLevel ) {
    Timer1.detachInterrupt();
    digitalWrite(TRIAC, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC, LOW);
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

  Serial.println(recipient);
  Serial.println(command);

  if (strcmp(recipient, deviceID) != 0) {
    Serial.println("ERROR: Not for me.");
    return;
  }

  if (strncmp(command, "fade ", 5) == 0) {
    char *param = strtok(command + 5, " ");
    byte target = (byte) strtol(param, (char **) NULL, 10);
    param = strtok(NULL, " ");
    unsigned long duration = strtoul(param, (char **) NULL, 10);
    Serial.print("Fade to ");
    Serial.print(target);
    Serial.print(" over ");
    Serial.print(duration / 100.0);
    Serial.println(" seconds");
    fader.start(dimLevel, target, duration * 10L, millis());
  }

  else if (strcmp(command, "turnOn") == 0) {
    Serial.println("On");
    deviceStatus = 1;
    light();
  }

  else if (strcmp(command, "turnOff") == 0) {
    Serial.println("Off");
    deviceStatus = 0;
    light();
  }

  else if (strcmp(command, "pulse") == 0) {
    Serial.println("Pulse");
    int current = dimLevel;
    dimLevel = 0;
    light();
    delay(1000);
    dimLevel = current;
    light();
  }

  else if (strncmp(command, "dim", 3) == 0) {
    int d = atoi(command+3);
    if (d < 0 || d > 255) {
      Serial.println("ERROR: Bad dim level.");
      return;
    }
    dimLevel = d;
    Serial.print("Dimming to ");
    Serial.println(d);
    light();
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
  Serial1.println("set sys autoconn 1");
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

void fixSSID() {
  for (i = 0; i < SSID_LENGTH; i++) {
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
