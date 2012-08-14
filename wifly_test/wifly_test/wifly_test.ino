/*
  WiFly test
 
 */

#include <WiFlySerial3.h>
#include <Streaming.h>

char passphrase[] = "claffey1";
char ssid[] = "Party$Central";
char ntp_server[] = "nist1-la.ustiming.org";

char server[] = "23.21.169.6";
int port = 1307;

#define REQUEST_BUFFER_SIZE 180
#define POST_BUFFER_SIZE 180
#define TEMP_BUFFER_SIZE 60

WiFlySerial wifi;
char bufRequest[REQUEST_BUFFER_SIZE];
char bufTemp[TEMP_BUFFER_SIZE];

void setup()  
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  wifi.setDebugChannel( (Print*) &Serial);
  
  // Wifi setup
  wifi.begin();
  wifi.setAuthMode( WIFLY_AUTH_WPA2_PSK );
  wifi.setJoinMode( WIFLY_JOIN_MANUAL );
  wifi.setDHCPMode( WIFLY_DHCP_ON );
  
  // Clear the network
  wifi.leave();
  
  // Join the new network
  wifi.setPassphrase(passphrase);
  if ( wifi.join(ssid) ) {
    Serial.println("Connected!");
  } else {
    Serial.println("Fail.");
    while (1); // Hang. TODO: Something better here
  }
  
  // Set NTP server
  wifi.setNTP(ntp_server);
  
  // Set the remote port
  wifi.setRemotePort(port);
  
  // Show WiFly status
  //wifi.getIP(bufRequest, REQUEST_BUFFER_SIZE);
  //wifi.getNetMask(bufRequest, REQUEST_BUFFER_SIZE);
  //wifi.getGateway(bufRequest, REQUEST_BUFFER_SIZE);
  //wifi.getDNS(bufRequest, REQUEST_BUFFER_SIZE);
  
  // Create connection
  if ( wifi.openConnection( server ) ) {
     wifi.print("{ \"deviceid\": \"Elroy\" }");
  }
  
}

void loop() // run over and over
{
  while (wifi.available()) {
    Serial.write(wifi.read());
  }
  
  if (Serial.available()) {
    wifi.write(Serial.read());
  }
}
