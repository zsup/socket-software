//
// Test for debugSerial port for Spark prototype.
// If it works, this should make a modified echo on the debugSerial port.
// The modification is that it'll always return echo+1
//

#include <SoftwareSerial.h>

#define RED            5       // Pin for red LED (PD5, pin 9, Arduino D5, PWM-capable)
#define BLUE           6       // Pin for blue LED (PD6, pin 10, Arduino D6, PWM-capable)
#define GREEN          9       // pin for green LED (PB1, pin 13, Arduino D9, PWM-capable)

#define DEBUG_RX       7       // Pin for software serial RX (PD7, pin 11, Arduino D7)
#define DEBUG_TX       8       // Pin for software serial TX (PB0, pin 12, Arduino D8)

#define BAUD           9600    // Serial communication speed

SoftwareSerial debugSerial(DEBUG_RX, DEBUG_TX);

void setup() {
  sei();                        // This is AVR code to enable interrupts
  debugSerial.begin(BAUD);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, HIGH);
  digitalWrite(BLUE, HIGH);
}

void loop() {
  if ( debugSerial.available() > 0 ) {
    char c = debugSerial.read();
    switch (c) {
      case 'r':
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, HIGH);
        digitalWrite(BLUE, HIGH);
        break;
      case 'g':
        digitalWrite(RED, HIGH);
        digitalWrite(GREEN, LOW);
        digitalWrite(BLUE, HIGH);
        break;
      case 'b':
        digitalWrite(RED, HIGH);
        digitalWrite(GREEN, HIGH);
        digitalWrite(BLUE, LOW);
        break;
      default:
        digitalWrite(RED, HIGH);
        digitalWrite(GREEN, HIGH);
        digitalWrite(BLUE, HIGH);
        break;
    }
  }
}
