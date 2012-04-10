
// (Based on Ethernet's WebClient Example)

#include <String.h>
#include <SPI.h>
#include <WiFly.h>

#include "Credentials.h"

#define BUFF_LEN 80
#define DEVICE_ID "Elroy"

const int requestInterval = 10000; // delay between requests
long lastAttemptTime = 0; // last time you connected to the server, in milliseconds
const int ledPin = 2; // the pin that the LED is attached to
int status = 0; // status of the device

byte server[] = { 23, 21, 169, 6 }; // Amazon EC2
// byte server[] = { 10, 0, 1, 25 }; // MacBook on local network

char bufferString[BUFF_LEN+1]; // string to hold the text from the server
int bufferLength = 0;

WiFlyClient client(server, 1307);  // WiFly client

void setup() {
  
  // initialize serial communication:
  Serial.begin(9600);
  
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);

  WiFly.begin();
  
  if (!WiFly.join(ssid, passphrase)) {
    Serial.println("Association failed.");
    while (1) {
      // Hang on failure.
    }
  }  
  
  connectToServer();
}

void loop() {
  if (client.connected()) {
    if (client.available()) {
      // if there are incoming bytes available
      // from the server, read them and print them:
      char c = client.read();
      
      // if you get a newline, clear the line and process the command:
      if (c == '\n') {
        process();
      }
      // add the incoming bytes to the end of line:
      else {
        bufferString[bufferLength] = c;
        bufferLength++;
      }
    }

    // as long as there are bytes in the serial queue, 
    // read them and send them out the socket
    while (Serial.available() > 0) {
      char inChar = Serial.read();
      client.print(inChar); 
    }
  }
  else if (millis() - lastAttemptTime > requestInterval) {
    // if you're not connected, and enough time has passed since
    // your last connection, then attempt to connect again:
    connectToServer();
  }
}

void connectToServer() {
  
  Serial.println("Connecting...");

  if (client.connect()) {
    client.print(DEVICE_ID);
    client.print('\n');
    Serial.println("Connected.");
  }
  else {
    Serial.println("Connection failed.");
  }
  
  // Note the time of the last connect
  lastAttemptTime = millis();
}

void process() {
  
  // if the current line ends with turnOn, activate the light
  if (strcmp(bufferString, "turnOn") == 0) {
    digitalWrite(ledPin, HIGH);
    status = 1;
    Serial.println("On");
  }
    
  // if the current line ends with turnOff, deactivate the light
  else if (strcmp(bufferString, "turnOff") == 0) {
    digitalWrite(ledPin, LOW);
    status = 0;
    Serial.println("Off");
  }
  
  // if the current line ends with "toggle", toggle the light
  else if (strcmp(bufferString, "toggle") == 0) {
    if ( status == 1 ) {
      digitalWrite(ledPin, LOW);
      status = 0;
    }
    else {
      digitalWrite(ledPin, HIGH);
      status = 1;
    }
    Serial.println("Toggle.");
  }
      
  // if the current line ends with "disconnectClient", disconnect the server
  else if (strcmp(bufferString, "disconnectClient") == 0) {
    client.stop();
    Serial.println("Disconnect.");
  }
  
  else {
    Serial.println("Unrecognized command.");
  }
  
  // Erase the buffer string
  int i;
  for (i = 0; i < BUFF_LEN; i++)
    bufferString[i] = '\0';
  bufferLength = 0;
}
