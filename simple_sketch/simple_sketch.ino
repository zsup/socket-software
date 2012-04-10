
// (Based on Ethernet's WebClient Example)

#include <SPI.h>
#include <WiFly.h>
// #include <aJSON.h>

#include "Credentials.h"

const int requestInterval = 10000; // delay between requests
long lastAttemptTime = 0; // last time you connected to the server, in milliseconds
const int ledPin = 2; // the pin that the LED is attached to

String deviceid = "Elroy"; // Device ID, string of characters

byte server[] = { 23, 21, 169, 6 }; // Amazon EC2
// byte server[] = { 10, 0, 10, 1 }; // MacBook on local network
// byte server[] = { 192, 168, 1, 14 }; // MacBook on Noah's network

String bufferString = ""; // string to hold the text from the server

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
      Serial.print(c);
      
      // if you get a newline, clear the line and process the command:
      if (c == '\n') {
        Serial.println("Processing command.");
        process();
      }
      // add the incoming bytes to the end of line:
      else {
        bufferString += c;
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
  
  Serial.println("connecting...");

  if (client.connect()) {
    Serial.println("connected");
    // client.print(jsonify("Device ID", deviceid));
    client.print(deviceid);
  } else {
    Serial.println("connection failed");
  }
  
  // Note the time of the last connect
  lastAttemptTime = millis();
}

void process() {
  
  // if the current line ends with HIGH, activate the LED
  if ( bufferString.startsWith("turnOn")) {
    Serial.println("Turning LED on.");
    digitalWrite(ledPin, HIGH);
  }
    
  // if the current line ends with LOW, deactivate the LED
  if ( bufferString.startsWith("turnOff")) {
    Serial.println("Turning LED off.");
    digitalWrite(ledPin, LOW);
  }
      
  // if the current line ends with EXIT, disconnect the server
  if ( bufferString.startsWith("disconnectClient")) {
    Serial.println("Disconnecting client.");
    client.stop();
  }
  
  // Erase the buffer string
  bufferString = "";
}

String jsonify (String key, String value) {
  String result = "";
  result = "{ " + key + " : " + value + " }";
  return result;
}
