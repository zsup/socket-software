/*
  Software serial multple serial test
 
 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.
 
 The circuit: 
 * RX is digital pin 2 (connect to TX of other device)
 * TX is digital pin 3 (connect to RX of other device)
 
 created back in the mists of time
 modified 9 Apr 2012
 by Tom Igoe
 based on Mikal Hart's example
 
 This example code is in the public domain.
 
 */


void setup()  
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial1.begin(9600);
  
  Serial.println("Serial initialized.");
  
  Serial1.print("$$$");
  delay(1000);
  Serial1.println("set wlan ssid Party$Central");
  Serial1.println("set ip host 23.21.169.6");
  Serial1.println("set ip remote 1307");
  Serial1.println("set sys autoconnect 1");
  Serial1.println("open");
  Serial1.println("exit");
  delay(2000);
  Serial1.println("{ \"deviceid\" : \"Elroy\" }");
  Serial.println("Finished setup");
  
}

void loop() // run over and over
{
  if (Serial1.available())
    Serial.write(Serial1.read());
  if (Serial.available())
    Serial1.write(Serial.read());
}
