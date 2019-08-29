#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// Calculate based on max input size expected for one command
#define INPUT_SIZE 30

// software serial pins
const byte RO_PIN = 8;
const byte DI_PIN = 9;
const byte XMIT_ENABLE_PIN = 10;
SoftwareSerial rs485(RO_PIN, DI_PIN); // receive pin, transmit pin

struct Message
{
  int address;
  char command[20];
  char info[20];
  char  available;
} ;

  Message tosendmessage;
  Message toreceivemessage;



// callbacks for the non-blocking RS485 library
size_t fWrite(const byte what)
{
  rs485.write(what);
}

int fAvailable()
{
  return rs485.available();
}

int fRead()
{
  return rs485.read();
}

// RS485 library instance
RS485 rs485Channel(fRead, fAvailable, fWrite, 50);

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }

  Serial.println(EEPROM.read(0));

  pinMode(XMIT_ENABLE_PIN, OUTPUT);
  rs485.begin(9600);
  // initialize the RS485 library
  rs485Channel.begin();
}

void loop() // run over and over
{
  
  if (rs485Channel.update()) {
    Serial.println("ble");
    memset (&toreceivemessage, 0, sizeof toreceivemessage);
    int len = rs485Channel.getLength ();
    if (len > sizeof toreceivemessage)
      len = sizeof toreceivemessage;
    memcpy(&toreceivemessage, rs485Channel.getData(), len);
    Serial.println(toreceivemessage.address);
    Serial.println(toreceivemessage.command);
    Serial.println(toreceivemessage.info);
    Serial.println("ble");
  }
    

  if (Serial.available()){
    Serial.println("bla");
    tosendmessage.address = EEPROM.read(0);
    // Get next command from Serial (add 1 for final 0)
    char input[INPUT_SIZE + 1];
    byte size = Serial.readBytes(input, INPUT_SIZE);
    // Add the final 0 to end the C string
    input[size] = 0;
    
    // Read each command pair 
    char* command = strtok(input, "&");
    while (command != 0)
    {
        // Split the command in two values
        char* separator = strchr(command, ':');
        if (separator != 0)
        {
            // Actually split the string in 2: replace ':' with 0
            *separator = 0;
            strcpy(tosendmessage.command,command);
            ++separator;
            strcpy(tosendmessage.info, separator);
    
            // Do something with servoId and position
        }
        // Find the next command in input string
        command = strtok(0, "&");
        digitalWrite(XMIT_ENABLE_PIN, HIGH); // enable sending
        rs485Channel.sendMsg((byte *)&tosendmessage, sizeof tosendmessage);
        digitalWrite(XMIT_ENABLE_PIN, LOW); // disable sending
    }


    Serial.println( sizeof tosendmessage);
    Serial.println(tosendmessage.address);
    Serial.println(tosendmessage.command);
    Serial.println(tosendmessage.info);
    Serial.println("bla");
  }
  
}
