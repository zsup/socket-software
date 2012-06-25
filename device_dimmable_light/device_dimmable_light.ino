
// Arduino code for a SWITCH-enabled dimmable lamp.
// Communicates over serial connection (in practice, an XBee transmitter).
//
// Available commands:
//   turnOn: Turns lamp on
//   turnOff: Turns lamp off
//   toggle: Toggles lamp
//   dimX: Dim the light to a certain level between 0 and 255
//   getdeviceStatus: Returns deviceStatus of device in JSON
//

#include <TimerOne.h>  // From http://www.arduino.cc/playground/Code/Timer1

const int AC_LOAD = 3; // the pin that the TRIAC control is attached to
const int INTERRUPT = 1; // the pin that the zero-cross is connected to
const int BAUD = 9600; // Serial communication speed
boolean deviceStatus = 0; // Status of the device upon initialization. It starts off.
int dimLevel = 255; // Dim level upon initialization.

String DEVICE_ID = "Elroy";
String DEVICE_TYPE = "LED"; // TODO: Should be "Dimmable Lamp". Will have to fix this once the server's updated

String bufferString; // string to hold the text from the server

volatile int i = 0; // Our counter
volatile boolean zc = 0; // Boolean to let us know whether we have crossed zero
volatile boolean triac = 0; // Boolean to store whether triac has been triggered
int freq = 31; // Delay for the frequency of power per step (using 256 steps)

void setup() {

  // initialize serial communication:
  Serial.begin(BAUD);
  Serial1.begin(BAUD);

  // initialize the LED pin as an output:
  pinMode(AC_LOAD, OUTPUT);
  
  // Attach interrupts
  attachInterrupt(INTERRUPT, zero_cross, RISING);
  Timer1.initialize(freq);
  Timer1.attachInterrupt(dim_check, freq);

  // Announce thyself
  Serial1.print("{ \"deviceid\" : \"");
  Serial1.print(DEVICE_ID);
  Serial1.print("\" , \"devicetype\" : \"");
  Serial1.print(DEVICE_TYPE);
  Serial1.print("\" , \"devicestatus\" : ");
  Serial1.print(deviceStatus);
  Serial1.print(" , \"dimval\" : ");
  Serial1.print(dimLevel);
  Serial1.print(" }");
  Serial1.print("\n");
}

void zero_cross() {
  zc = 1;
}

void dim_check() {
  if ( zc == 1 ) {
    if ( deviceStatus == 1) {
      if (i >= 255 - dimLevel ) {
        if ( triac == 1 ) {
          digitalWrite(AC_LOAD, LOW);
          triac = 0;
          i = 0;
          zc = 0;
          Serial.print(0);
        } else {
          digitalWrite(AC_LOAD, HIGH);
          triac = 1;
          Serial.print(1);
        }
      } else {
        i++;
      }
    } else {
      if (i >= 255 - dimLevel ) {
        i = 0;
        zc = 0;
      } else {
        i++;
      }
    }
  }
}

void loop() {
  // as long as there are bytes in the serial queue, read them
  while (Serial1.available() > 0) {
    char c = Serial1.read();

    // if you get a newline, clear the line and process the command:
    if (c == '\n') {
      process(bufferString);
      bufferString = "";
    }
    // add the incoming bytes to the end of line:
    else {
      bufferString += c;
    }
  }
}

void process(String message) {
  
  message.trim();
  
  int separator = message.indexOf(',');
  
  if (separator == -1) {
    Serial.println("Error; incorrect syntax, no comma found");
  }
  else {
    String command = message.substring(separator+1);
    String recipient = message.substring(0,separator);
    Serial.print("Recipient: ");
    Serial.println(recipient);
    Serial.print("Command: ");
    Serial.println(command);
    
    if ( recipient == DEVICE_ID ) {
      if (command == "turnOn" ) {
        Serial.println("On.");
        deviceStatus = 1;
      }
      else if (command == "turnOff" ) {
        Serial.println("Off.");
        deviceStatus = 0;
      }
      else if (command == "getStatus" ) {
        Serial.println("Returning status.");
        Serial1.print("{ \"deviceid\" : \"");
        Serial1.print(DEVICE_ID);
        Serial1.print("\" , \"devicestatus\" : ");
        Serial1.print(deviceStatus);
        Serial1.print(" , \"dimval\" : ");
        Serial1.print(dimLevel);
        Serial1.println(" }");
      }
      else if (command.substring(0,3) == "dim" ) {
        command = command.substring(3);
        char buf[command.length()+1];
        command.toCharArray(buf, command.length()+1);
        int req = atoi(buf);
        if ( req >= 0 && req <= 255 ) {
          dimLevel = req;
          Serial.print("Dim to ");
          Serial.println(dimLevel);
        }
        else {
          Serial.println("Error: Not a valid dim level.");
        }
      }
    }
    else {
      Serial.println("Not for me.");
    }
  }
  
  
  /*
  if ( i == BUFF_LEN ) {
    Serial.println("Error; incorrect syntax, no comma found");
  }

  else if (strncmp(bufferString, DEVICE_ID, i) == 0) {
    i++;
    char* command = bufferString+i;

    Serial.println("This is for me! I'll do something with it.");
    Serial.print("Command: ");
    Serial.println(command);

    // if the command is turnOn, activate the light
    if (strncmp(command, "turnOn", 6) == 0) {
      Serial.println("On");
    }

    // if the command is turnOff, deactivate the light
    else if (strcmp(command, "turnOff") == 0) {
      Serial.println("Off");
    }

    // if the command is "toggle", toggle the light
    else if (strcmp(command, "toggle") == 0) {
      Serial.println("Toggle.");
    }
    
    // 
    else if (strncmp(command, "dim", 3) == 0) {
      char* endptr;
      command = command + 3;
      int input = strtol (command, &endptr, 10);
      if ( input > 0 && input < 256 ) {
        dimLevel = input;
        Serial.print("Dim level: ");
        Serial.println(dimLevel);
      }
      else {
        Serial.println("Error: Bad dim level");
      }
    }
      
    // if the command is "getStatus", respond with deviceStatus
    else if (strcmp(command, "getStatus") == 0) {
      Serial1.print("{ \"deviceid\" : \"");
      Serial1.print(DEVICE_ID);
      Serial1.print("\" , \"devicestatus\" : ");
      Serial1.print(deviceStatus);
      Serial1.println(" }");
    }

    else {
      Serial.println("Unrecognized command.");
    }
  }
  else {
    Serial.println("Not for me.");
  }

  // Erase the buffer string
  for (i = 0; i < BUFF_LEN; i++)
    bufferString[i] = '\0';
  bufferLength = 0;
  */
}
