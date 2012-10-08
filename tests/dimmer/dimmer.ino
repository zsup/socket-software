//
// Test for the zero cross detector on the Spark prototype.
// If it works, the red LED should flash at 60Hz.
//

#include "TimerOne.h"

#define INTERRUPT      0       // Zero-cross interrupt (PD2, pin 32, Arduino D2, interrupt 0)
#define TRIAC          3       // TRIAC control pin (PD3, pin 1, Arduino D3)
#define RED            5       // Pin for red LED (PD5, pin 9, Arduino D5, PWM-capable)
#define TIMER_MICROSECONDS  32L
#define DIM_MIN        0
#define DIM_MAX        255

volatile int dimLevel = 0;                    // Dim level upon initialization. Defaults to off (0).
volatile int t = 0;                           // Our counter for zero cross events

void setup() {
  sei();                        // This is AVR code to enable interrupts
  attachInterrupt(INTERRUPT, zero_cross, RISING);
  pinMode(TRIAC, OUTPUT);
  pinMode(RED, OUTPUT);
  digitalWrite(RED, HIGH);
  Timer1.initialize(TIMER_MICROSECONDS);
}

void loop() {
  dimLevel = 0;
  delay(1000);
  dimLevel = 50;
  delay(1000);
  dimLevel = 100;
  delay(1000);
  dimLevel = 150;
  delay(1000);
  dimLevel = 200;
  delay(1000);
  dimLevel = 250;
  delay(1000);
}

void zero_cross() {
  t = 0;

  if (dimLevel < DIM_MIN + 10) {
    digitalWrite(TRIAC, LOW);
  }

  else if (dimLevel > DIM_MAX - 10) {
    digitalWrite(TRIAC, HIGH);
  }

  else {
    Timer1.attachInterrupt(dim_check);
  }
}

void dim_check() {
  // First, increment.
  t++;

  // Check to see if the counter has reached the right point. If it has, trip the TRIAC.
  if ( t >= DIM_MAX - dimLevel ) {
    Timer1.detachInterrupt();
    digitalWrite(TRIAC, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC, LOW);
  }
}
