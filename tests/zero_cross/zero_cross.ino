//
// Test for the zero cross detector on the Spark prototype.
// If it works, the red LED should flash at 60Hz.
//

#define INTERRUPT      0       // Zero-cross interrupt (PD2, pin 32, Arduino D2, interrupt 0)
#define RED            5       // Pin for red LED (PD5, pin 9, Arduino D5, PWM-capable)


void setup() {
  sei();                        // This is AVR code to enable interrupts
  attachInterrupt(INTERRUPT, zero_cross, RISING);
  pinMode(RED, OUTPUT);
  digitalWrite(RED, HIGH);
}

void loop() {
  
}

void zero_cross() {
  digitalWrite(RED, LOW);
  delay(5);
  digitalWrite(RED, HIGH);
}
