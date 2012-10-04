/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led1 = 5;
int led2 = 6;
int led3 = 9;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
}

// the loop routine runs over and over again forever:

void loop() {
  analogWrite(led1, 100);
  delay(1000);
  analogWrite(led1, 255);
  delay(1000);
  analogWrite(led1, 0);
  delay(1000);
}
