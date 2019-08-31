#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Defines
#define INPUT_SIZE 50
#define NUMBEROFGPIOS 20
#define INVALID_PORT 0x04

//Structs
// the data we broadcast to each other device changes
struct Message
{
  int address;
  char command[20];
  char info[20];
  char available;
} message;

//Port value and mode
struct GpioStatus
{
  // 0 - input
  // 1 - output
  //
  uint8_t Mode;
  uint8_t value;
  uint8_t changed;
} gpioports[NUMBEROFGPIOS];

//For Porto..
const int sensorIn = A7;
const int mVperAmp = 100; // use 100 for 20A Module and 66 for 30A Module
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

// from EEPROM
byte myAddress;       // who we are
byte numberOfDevices; // maximum devices on the bus

//RS485
const unsigned long RS485_BAUD_RATE = 9600;
// software serial pins
const byte RO_PIN = 8;
const byte DI_PIN = 9;
// transmit enable
const byte RS485_ENABLE = 10;
uint8_t gpioChanged = 0 ;

SoftwareSerial rs485(RO_PIN, DI_PIN); // receive pin, transmit pin

// callbacks for the non-blocking RS485 library
size_t fWrite(const byte what)
{
  return rs485.write(what);
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

//Misc
const int BUTTON_IN = 3;
const int BUTTON_OUT = 2;
const float TIME_PER_BYTE = 1.0 / (RS485_BAUD_RATE / 10.0); // seconds per sending one byt

// action pins (demo)
const int REED_PIN = 2; // Pin connected to reed switch
const int LED_PIN = 13; // LED pin - active-high
const byte ENABLE_PIN = 4;
String inString = "";

//Serial Connection Defines
const unsigned long BAUD_RATE = 9600;

// the setup function runs once when you press reset or power the board
void setup()
{
  //Initialize GpioStatus Array
  for (int i; i < NUMBEROFGPIOS; i++)
  {
    gpioports[i].Mode = INPUT;
    gpioports[i].value = 0;
  }

  // initialize serial communication at 9600 bits per second:
  Serial.begin(BAUD_RATE);
  gpioports[0].Mode = INVALID_PORT;
  gpioports[1].Mode = INVALID_PORT;

  //Read From EEPROM
  myAddress = EEPROM.read(0);
  numberOfDevices = EEPROM.read(1);

  //RS485 Configuration
  rs485.begin(RS485_BAUD_RATE);
  rs485Channel.begin();
  //Pin Mode
  pinMode(RS485_ENABLE, OUTPUT);
  gpioports[RO_PIN].Mode = INVALID_PORT;
  gpioports[DI_PIN].Mode = INVALID_PORT;
  gpioports[RS485_ENABLE].Mode = INVALID_PORT;

  // demo action pins
  //pinMode(ENABLE_PIN, OUTPUT);
  // Since the other end of the reed switch is connected to ground, we need
  // to pull-up the reed switch pin internally.

  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, HIGH);

  pinMode(ENABLE_PIN, OUTPUT); // driver output enable
  pinMode(BUTTON_IN, INPUT);
  pinMode(BUTTON_OUT, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(ENABLE_PIN, LOW);
  digitalWrite(BUTTON_OUT, LOW);
}

//
// RS485
//

// Here to process an incoming message
void processRS48Message(Message *message)
{
  memset(message, 0, sizeof(*message));
  uint32_t len = rs485Channel.getLength();
  if (len > sizeof(*message))
    len = sizeof(*message);
  memcpy(message, rs485Channel.getData(), len);
} // end of processMessage

// Here to send our own message
void sendRS485Message(Message message)
{
  digitalWrite(RS485_ENABLE, HIGH); // enable sending
  rs485Channel.sendMsg((byte *)&message, sizeof message);
  digitalWrite(RS485_ENABLE, LOW); // disable sending
} // end of sendMessage

//
// Serial
//

void sendSerialData(Message message)
{
  Serial.print(message.address);
  Serial.print("_");
  Serial.print(message.command);
  Serial.print("_");
  Serial.println(message.info);
}

//For now it will only accept one communication at a time
//comand:info (missing reading address)
void checkSerialData(Message *message)
{
  message->address = EEPROM.read(0);
  // Get next command from Serial (add 1 for final 0)
  char input[INPUT_SIZE + 1];
  byte size = Serial.readBytes(input, INPUT_SIZE);
  // Add the final 0 to end the C string
  input[size] = 0;

  // Read each command pair
  char *command = strtok(input, "&");
  while (command != 0)
  {
    // Split the command in two values
    char *separator = strchr(command, ':');
    if (separator != 0)
    {
      // Actually split the string in 2: replace ':' with 0
      *separator = 0;
      strcpy(message->command, command);
      ++separator;
      strcpy(message->info, separator);

      // Do something with servoId and position
    }
    // Find the next command in input string
    command = strtok(0, "&");
    //TODO: should set more than 1 message
  }
  message->available = 1;
}

//
// GPIO
//

float getVPP()
{
  float result;

  int readValue;       //value read from the sensor
  int maxValue = 0;    // store max value here
  int minValue = 1024; // store min value here

  uint32_t start_time = millis();
  while ((millis() - start_time) < 100) //sample for 1 Sec
  {
    readValue = analogRead(sensorIn);
    // see if you have a new maxValue
    if (readValue > maxValue)
      maxValue = readValue;
    if (readValue < minValue)
      minValue = readValue;
  }

  // Subtract min from max
  result = ((maxValue - minValue) * 5.0) / 1024.0;

  return result;
}

void portoHelper()
{
  int working = digitalRead(BUTTON_IN);
  //For Manual Work
  if (working)
  {
    digitalWrite(LED_BUILTIN, HIGH);

    Voltage = getVPP();
    VRMS = (Voltage / 2.0) * 0.707;
    AmpsRMS = (VRMS * 1000) / mVperAmp;

    if (AmpsRMS < 1)
    {
      Serial.write("1_portohelper_opening");
      delay(2000);
      digitalWrite(BUTTON_OUT, HIGH);
      delay(5000);
    }
    digitalWrite(BUTTON_OUT, LOW);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void checkGPIOutput(Message *message)
{
}

void prossesGPIOInput(Message message)
{
  for (uint8_t i = 0; i < NUMBEROFGPIOS; i++)
  {
    if (gpioports[i].Mode == INPUT )
    {
      int value = digitalRead(i);
      if (value != gpioports[i].value)
      {
        gpioports[i].value = value;
        gpioports[i].changed = 1;
        gpioChanged = 1;
      }
    }
  }
}

// the loop function runs over and over again forever
void loop()
{
  // Create message
  //Check Serial connection for data (send always)
  message.address = 0;
  message.available = 0;
  // Serial.println(message.address);

  //////
  // Create Message
  //////

  //Data can comme from two places..
  if (Serial.available())
  {
    checkSerialData(&message);
  } //end serial.available

  //
  checkGPIOutput(&message);

  if (rs485Channel.update())
  {
    processRS48Message(&message);
  } // end of message completely received

  //TODO add: myAddress == 1 &&
  //Check RS connection for data
  if (message.available)
  {
  }

  //Send Message to serial and RS485
  //TODO serial only on device 1?
  if (message.available || gpioChanged )
  {
    sendSerialData(message);
    sendRS485Message(message);
    gpioChanged = 0;
  }


} //end loop