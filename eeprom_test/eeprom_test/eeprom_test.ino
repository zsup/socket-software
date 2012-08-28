// Test for EEPROM functionality for Arduino.
//
//

#include <EEPROM.h>

int SSID_LENGTH = 32;
int PWORD_LENGTH = 32;

char test1[32] = "beluga";
char test2[32] = "whale";

char ssid[32] = "";
char pword[32] = "";
int auth = 0;
int hasInfo = 0;

int deviceStatus = 0;
int dimLevel = 255;

int EEPROMMIN = 0;
int EEPROMMAX = 511;

int STATUS_ADDR = 0;
int DIMLEVEL_ADDR = 1;
int HASINFO_ADDR = 2;
int SSID_ADDR = 3;
int PWORD_ADDR = 36;
int AUTH_ADDR = 69;

void setup() {
  Serial.begin(9600);
  
  hasInfo = EEPROM.read(HASINFO_ADDR);
  if (hasInfo) {
    for (int i = 0; i < SSID_LENGTH; i++) {
      ssid[i] = EEPROM.read(SSID_ADDR + i);
    }
    for (int i = 0; i < PWORD_LENGTH; i++) {
      pword[i] = EEPROM.read(PWORD_ADDR + i);
    }
    auth = EEPROM.read(AUTH_ADDR);
  }
  
  eepromtoChar(SSID_ADDR, ssid, SSID_LENGTH);
  eepromtoChar(PWORD_ADDR, pword, PWORD_LENGTH);
  
  delay(5000);
  
  Serial.print("The SSID is: ");
  Serial.println(ssid);
  Serial.print("The password is: ");
  Serial.println(pword);
}

void loop () {
  ;
}

void clearEEPROM () {
  for (int i=EEPROMMIN; i <= EEPROMMAX; i++) {
    EEPROM.write(i, 0);
  }
}

void readEEPROMtoSerial () {
  for (int i = EEPROMMIN; i <= EEPROMMAX; i++) {
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
