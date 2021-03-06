/*
  Button and LED test for the Spark prototype.
  
  When the button is pressed, the LED should turn on white. When the button is not pressed, the LED should be off.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led1 = 5;
int led2 = 6;
int led3 = 9;
int button = 4;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(button, INPUT_PULLUP);
}

// the loop routine runs over and over again forever:

void loop() {
  if (digitalRead(button) == LOW) {
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
  } else {
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
    digitalWrite(led3, HIGH);
  }
}
