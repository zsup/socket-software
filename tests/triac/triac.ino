//
// Test for the TRIAC.
// If it works, the light should blink (1s cycle)
//

#define TRIAC          3       // TRIAC control pin (PD3, pin 1, Arduino D3)

void setup() {
  pinMode(TRIAC, OUTPUT);
}

void loop() {
  digitalWrite(TRIAC, HIGH);
  delay(1000);
  digitalWrite(TRIAC, LOW);
  delay(1000);
}
