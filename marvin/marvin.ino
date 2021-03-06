// Arduino code for the Spark Orb Prototype.
// Written by Zach Supalla and Zachary Crockett for Hex Labs, Inc.
// Copyright Hex Labs, Inc. 2013
// All rights reserved.
//
// Works with Marvin-HAXLR8R+60 (hacked party ball)
//
// Orb has the following components:
//   Arduino Uno
//   Roving Networks RN-171 Wi-Fi module
//   ATtiny pins controlling R, G, & B LEDs jumped to ATmega
//
// Available commands:
//   turnOn: Turns lamp on
//   turnOff: Turns lamp off
//   toggle: Toggles lamp
//   dimX: Dim the light to a certain level between 0 and 255
//   getStatus: Returns status of device in JSON
//   fade X Y: Fade to X (0-12) over Y (0-600000) milliseconds
//   ledRRGGBB: Change LEDs immediately to the given RGB hex value
//
// Known issues:
//   - There's a long delay during boot-up
//   - After 10 minutes or so, the device starts acting wacky and needs to be rebooted
//   - The whole 'alternate start-up lighting' thing doesn't really work
//
// v0.4
// 03/20/2013

// Libraries. Need a timer for dimming with PWM, and EEPROM for saving.
#include <EEPROM.h>
#include "TimerOne.h"  // From http://www.arduino.cc/playground/Code/Timer1
#include "Fader.h"

#define RED            3       // the pin that the RED LEDs are attached to
#define GREEN          5       // the pin that the RED LEDs are attached to
#define BLUE           6       // the pin that the RED LEDs are attached to
#define INTERRUPT      1       // the pin that the zero-cross is connected to
#define BAUD           9600    // Serial communication speed

#define SSID_LENGTH    32
#define PWORD_LENGTH   32
#define BUFFER_LENGTH  256

#define EEPROM_MIN     0
#define EEPROM_MAX     511

#define DIM_MIN        0
#define DIM_MAX        255
#define LED_MAX        255

#define STATUS_ADDR    0
#define HASINFO_ADDR   2
#define SSID_ADDR      3
#define PWORD_ADDR     36
#define AUTH_ADDR      69

// slightly less than 1/256th of a half period of the alternating current, in microseconds
// this is 60 Hz, but works for 50 Hz overseas, where the value is 38
#define TIMER_MICROSECONDS  32L

volatile int dimLevel = 0;  // Dim level upon initialization. Defaults to off (0).
volatile int last = 255;          // Variable to store the last lighting status. Used when turned off.

volatile int t = 0; // Our counter for zero cross events

// Device info
char deviceID[32] = "marvin"; // Unique ID for the device
char deviceType[32] = "Light"; // Devicetype; light

// Network stuff
char ssid[SSID_LENGTH] = "";           // TODO: Right number of characters?
char pword[PWORD_LENGTH] = "";          // TODO: Right number of characters?
int auth = 0;
boolean hasInfo = 0;

char server[] = "54.235.79.249";
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

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  set_leds(0, 0, 0);

  Timer1.initialize(TIMER_MICROSECONDS);
  attachInterrupt(INTERRUPT, zero_cross, RISING);

  // Check the EEPROM for the last status, and flip it.
  int deviceStatus = EEPROM.read(STATUS_ADDR);
  if ( deviceStatus == 1 )
  {
    dimLevel = 255;
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
* LED FUNCTIONS
***********************/

void led(char ledval[]) {
  char bytestr[3];
  bytestr[2] = '\0';

  strncpy(bytestr, ledval, 2);
  byte redval = (byte) strtol(bytestr, (char **) NULL, 16);

  strncpy(bytestr, ledval + 2, 2);
  byte greenval = (byte) strtol(bytestr, (char **) NULL, 16);

  strncpy(bytestr, ledval + 4, 2);
  byte blueval = (byte) strtol(bytestr, (char **) NULL, 16);

  set_leds(redval, greenval, blueval);
}

void set_leds(byte red, byte green, byte blue) {
  analogWrite(RED, LED_MAX - red);
  analogWrite(GREEN, LED_MAX - green);
  analogWrite(BLUE, LED_MAX - blue);
}

/***********************
 * LIGHTING FUNCTIONS
 ***********************/

// Function called when the zero-cross happens.
// If the light is either on or off, just write high or low to the LEDs.
// If the light should be dimmed, attach the timer.
void zero_cross() {
  t = 0;

  if (dimLevel < DIM_MIN + 10) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }

  else if (dimLevel > DIM_MAX - 10) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, HIGH);
  }

  else {
    Timer1.attachInterrupt(dim_check);
  }

  if (fader.is_fading()) {
    dimLevel = fader.current_level(millis());
  }
}

void dim_check() {
  // First, increment.
  t++;

  // Check to see if the counter has reached the right point. If it has, trip the TRIAC.
  if ( t >= DIM_MAX - dimLevel ) {
    Timer1.detachInterrupt();
    //digitalWrite(TRIAC, HIGH);
    //delayMicroseconds(10);
    //digitalWrite(TRIAC, LOW);
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

  if (strncmp(command, "fade ", 5) == 0) {
    char *param = strtok(command + 5, " ");
    byte target = (byte) strtol(param, (char **) NULL, 10);
    param = strtok(NULL, "\0");
    unsigned long duration = strtoul(param, (char **) NULL, 10);
    Serial.print("Fade to ");
    Serial.print(target);
    Serial.print(" over ");
    Serial.print(duration);
    Serial.println(" milliseconds");
    fader.start(dimLevel, target, duration, millis());
  }

  else if (strcmp(command, "turnOn") == 0) {
    Serial.println("On");
    dimLevel = 255;
  }

  else if (strcmp(command, "turnOff") == 0) {
    Serial.println("Off");
    last = dimLevel;
    dimLevel = 0;
  }

  else if (strcmp(command, "pulse") == 0) {
    Serial.println("Pulse");
    last = dimLevel;
    // Pulse up if the light is dim; pulse down if the light is bright.
    if (last > 80) {
      dimLevel = 0;
    }
    else {
      dimLevel = 255;
    }
    delay(1000);
    dimLevel = last;
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
  }

  else if (strncmp(command, "led", 3) == 0) {
    led(command+3);
    Serial.print("Changed LED color");
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
  Serial.print("$$$");
  delay(1000);
  Serial.print("set wlan ssid SWITCH_");
  Serial.println(deviceID);
  delay(500);
  Serial.println("set wlan channel 1");
  delay(500);
  Serial.println("set wlan join 4");
  delay(500);
  Serial.println("set ip address 169.254.1.1");
  delay(500);
  Serial.println("set ip netmask 255.255.0.0");
  delay(500);
  Serial.println("set ip dhcp 0");
  delay(500);
  Serial.println("save");
  delay(500);
  Serial.println("reboot");
}

void connectToServer() {
  // Connect to our server.
  Serial.print("$$$");
  delay(1000);
  Serial.print("set wlan ssid ");
  Serial.println(ssid);
  delay(500);
  Serial.print("set wlan phrase ");
  Serial.println(pword);
  delay(500);
  Serial.print("set wlan auth ");
  Serial.println(auth);
  delay(500);
  Serial.println("set wlan channel 0");
  delay(500);
  Serial.println("set wlan join 1");
  delay(500);
  Serial.println("set ip dhcp 1");
  delay(500);
  Serial.print("set ip host ");
  Serial.println(server);
  delay(500);
  Serial.print("set ip remote ");
  Serial.println(port);
  delay(500);
  Serial.println("set sys autoconn 1");
  delay(500);
  Serial.println("open");
  delay(500);
  Serial.println("save");
  delay(500);
  Serial.println("reboot");
  delay(5000);
  sendStatus();
}

void sendStatus() {
  Serial.println();
  Serial.print("{ \"deviceid\" : \"");
  Serial.print(deviceID);
  Serial.print("\" , \"dimval\" : ");
  Serial.print(dimLevel);
  Serial.print(" , \"ssid\" : \"");
  Serial.print(ssid);
  Serial.print("\" , \"pword\" : \"");
  Serial.print(pword);
  Serial.print("\" , \"auth\" : \"");
  Serial.print(auth);
  Serial.println("\" }");
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
