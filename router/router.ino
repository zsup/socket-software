
// (Based on Ethernet's WebClient Example)

#include <String.h>
#include <SPI.h>
#include <WiFly.h>

#include "Credentials.h"

const int requestInterval = 10000; // delay between requests
long lastAttemptTime = 0; // last time you connected to the server, in milliseconds

// byte server[] = { 23, 21, 169, 6 }; // Amazon EC2
byte server[] = { 10, 0, 1, 3 }; // MacBook on local network

WiFlyClient client(server, 1307);  // WiFly client

void setup() {
  
  // initialize serial communication:
  Serial.begin(9600);

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
    client.print("Elroy\n");
    Serial.println("Connected.");
  }
  else {
    Serial.println("Connection failed.");
  }
  
  // Note the time of the last connect
  lastAttemptTime = millis();
}
