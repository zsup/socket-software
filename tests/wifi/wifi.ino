//
// Test for the Wi-Fi module.
// Must have already tested debugSerial.
//
// If it's working, when the Serial user sends an endline,
// the Wi-Fi module will return all of the module's current values.
//

#include <SoftwareSerial.h>

#define DEBUG_RX       7       // Pin for software serial RX (PD7, pin 11, Arduino D7)
#define DEBUG_TX       8       // Pin for software serial TX (PB0, pin 12, Arduino D8)

#define BAUD           9600    // Serial communication speed

SoftwareSerial debugSerial(DEBUG_RX, DEBUG_TX);

void setup()
{
  Serial.begin(BAUD);
  debugSerial.begin(BAUD);
  
  debugSerial.println("Serial ready!");
}

void loop() {
  if ( debugSerial.available() ) {
    char c = debugSerial.read();
    if (c == 20) {
      debugSerial.println("Testing communications with Wi-Fi.");
    }
    c++;
    debugSerial.print(c);
    
  }
    
  if ( Serial.available() ) {
    char c = Serial.read();
    debugSerial.print(c);
  }
}
/*
void test_wifi() {
  debugSerial.println("Testing communications with Wi-Fi.");
  Serial.print("$$$");
  delay(1000);
  Serial.println("get everything");
}
*/
