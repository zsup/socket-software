
// Arduino code for a SWITCH-enabled lamp.
// Communicates over serial connection (in practice, an XBee transmitter).
// Available commands:
//   turnOn: Turns lamp on
//   turnOff: Turns lamp off
//   toggle: Toggles lamp
//   dimX: Dim the light to a certain level between 0 and 255
//   getdeviceStatus: Returns deviceStatus of device in JSON
//

const int AC_LOAD = 3; // the pin that the TRIAC control is attached to
const int INTERRUPT = 1;
const int BAUD = 9600;
int deviceStatus = 0;
int dimLevel = 250;

String DEVICE_ID = "Elroy";
String DEVICE_TYPE = "LED"; // Will have to fix this once the server's updated

String bufferString; // string to hold the text from the server

void setup() {

  // initialize serial communication:
  Serial.begin(BAUD);
  Serial1.begin(BAUD);

  // initialize the LED pin as an output:
  pinMode(AC_LOAD, OUTPUT);

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
  int dimtime = (31.5*(261-dimLevel));  // This math might change for different lights
  delayMicroseconds(dimtime);    // Off cycle
  digitalWrite(AC_LOAD, HIGH);   // triac firing
  delayMicroseconds(8.33);         // triac On propogation delay
  digitalWrite(AC_LOAD, LOW);    // triac Off
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
        attachInterrupt(INTERRUPT, zero_cross, RISING);
        Serial.println("On.");
        deviceStatus = 1;
      }
      else if (command == "turnOff" ) {
        detachInterrupt(INTERRUPT);
        Serial.println("Off.");
        deviceStatus = 0;
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
