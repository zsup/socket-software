//
// Test for debugSerial port for Spark prototype.
// If it works, this should make a modified echo on the debugSerial port.
// The modification is that it'll always return echo+1
//

#include <SoftwareSerial.h>

#define RED            5

#define DEBUG_RX       7       // Pin for software serial RX (PD7, pin 11, Arduino D7)
#define DEBUG_TX       8       // Pin for software serial TX (PB0, pin 12, Arduino D8)

#define BAUD           4800    // Serial communication speed

SoftwareSerial debugSerial(DEBUG_RX, DEBUG_TX);

void setup() {
  sei();                        // This is AVR code to enable interrupts
  // pinMode(DEBUG_RX, INPUT);
  // pinMode(DEBUG_TX, OUTPUT);
  debugSerial.begin(BAUD);
  pinMode(RED, OUTPUT);
  digitalWrite(RED, HIGH);
}

void loop() {
  if ( debugSerial.available() > 0 ) {
    char c = debugSerial.read();
    debugSerial.print("Found: ");
    debugSerial.println(c);
  }
}
