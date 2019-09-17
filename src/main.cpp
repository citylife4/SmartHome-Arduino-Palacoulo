#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Defines
#define INPUT_SIZE 50
#define NUMBEROFGPIOS 20
#define INVALID_PORT 0x04
#define PCADDR 0
#define MASTERADDR 1

//Structs
// the data we broadcast to each other device changes
// from_to_command_GPIOaddr_value
struct Message
{
  uint8_t from;
  uint8_t to;
  uint8_t command;
  uint8_t GPIOaddr;
  uint8_t value;
  char available;
};

//

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

// from EEPROM
byte myAddress;       // who we are
byte numberOfDevices; // maximum devices on the bus
byte inPorto;         // maximum devices on the bus

//RS485
const unsigned long RS485_BAUD_RATE = 9600;
// software serial pins
const byte RO_PIN = 8;
const byte DI_PIN = 9;
// transmit enable
const byte RS485_ENABLE = 10;
uint8_t gpioChanged = 0;

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

//For Porto..
const int DOOR_BELTSENSOR = A7;
const int mVperAmp = 100; // use 100 for 20A Module and 66 for 30A Module
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
const int DOOR_BUTTOM = 3;
const int DOOR_RELAY = 2;

// action pins (demo)
const int REED_PIN = 2; // Pin connected to reed switch
const int LED_PIN = 13; // LED pin - active-high
const byte ENABLE_PIN = 4;

//Serial Connection Defines
const unsigned long SERIAL_BAUD_RATE = 9600;

//State machine
enum State_enum
{
  SET,
  GET,
  CONFIG
};
//enum Sensors_enum {NONE, SENSOR_RIGHT, SENSOR_LEFT, BOTH};

// the setup function runs once when you press reset or power the board
void setup()
{
  //Initialize GpioStatus Array
  for (int i = 0; i < NUMBEROFGPIOS; i++)
  {
    gpioports[i].Mode = INPUT;
    gpioports[i].value = digitalRead(i);
  }

  // initialize serial communication at 9600 bits per second:
  Serial.begin(SERIAL_BAUD_RATE);
  gpioports[0].Mode = INVALID_PORT;
  gpioports[1].Mode = INVALID_PORT;

  //Read From EEPROM
  myAddress = EEPROM.read(0);
  numberOfDevices = EEPROM.read(1);
  inPorto = EEPROM.read(2);

  //RS485 Configuration
  rs485.begin(RS485_BAUD_RATE);
  rs485Channel.begin();
  //Pin Mode
  pinMode(RS485_ENABLE, OUTPUT);
  gpioports[RO_PIN].Mode = INVALID_PORT;
  gpioports[DI_PIN].Mode = INVALID_PORT;
  gpioports[RS485_ENABLE].Mode = INVALID_PORT;

  if (inPorto)
  {
    pinMode(ENABLE_PIN, OUTPUT); // driver output enable
    pinMode(DOOR_BUTTOM, INPUT);
    pinMode(DOOR_RELAY, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    //gpioports[ENABLE_PIN].Mode = INVALID_PORT;
    //gpioports[DOOR_BUTTOM].Mode = INVALID_PORT;
    //gpioports[DOOR_RELAY].Mode = INVALID_PORT;
    //gpioports[DOOR_BELTSENSOR].Mode = INVALID_PORT;
    //gpioports[LED_BUILTIN].Mode     = INVALID_PORT;

    digitalWrite(DOOR_RELAY, LOW);
  }
}

//
// RS485
//

// Here to process an incoming message
uint8_t processRS48Message(Message *message)
{
  memset(message, 0, sizeof(*message));
  uint32_t len = rs485Channel.getLength();
  if (len > sizeof(*message))
    len = sizeof(*message);
  memcpy(message, rs485Channel.getData(), len);
  message->available = 1;
  return 1;
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
  Serial.print(message.from);
  Serial.print("_");
  Serial.print(message.to);
  Serial.print("_");
  Serial.print(message.command);
  Serial.print("_");
  Serial.print(message.GPIOaddr);
  Serial.print("_");
  Serial.println(message.value);
}

void cleanMessage(Message *message)
{
  message->from = 0;
  message->to = 0;
  message->command = 0;
  message->GPIOaddr = 0;
  message->value = 0;
}

//For now it will only accept one communication at a time
//comand:info (missing reading address)
uint8_t checkSerialData(Message *message)
{
  // Get next command from Serial (add 1 for final 0)
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;
  char receivedChars[INPUT_SIZE + 1];
  char tempChars[INPUT_SIZE + 1];

  while (Serial.available() > 0)
  {
    rc = Serial.read();

    if (recvInProgress == true)
    {
      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= INPUT_SIZE + 1)
        {
          ndx = INPUT_SIZE;
        }
      }
      else
      {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
      }
    }

    else if (rc == startMarker)
    {
      recvInProgress = true;
    }
  }
  strcpy(tempChars, receivedChars);
  char *strtokIndx; // this is used by strtok() as an index
                    // this temporary copy is necessary to protect the original data
                    //   because strtok() used in parseData() replaces the commas with \0

  message->available = 1;
  // Add the final 0 to end the C string

  strtokIndx = strtok(tempChars, "_"); // get the first part - the string
  message->from = atoi(strtokIndx);
  strtokIndx = strtok(NULL, "_"); // get the first part - the string
  message->to = atoi(strtokIndx);
  strtokIndx = strtok(NULL, "_"); // get the first part - the string
  message->command = atoi(strtokIndx);
  strtokIndx = strtok(NULL, "_"); // get the first part - the string
  message->GPIOaddr = atoi(strtokIndx);
  strtokIndx = strtok(NULL, "_"); // get the first part - the string
  message->value = atoi(strtokIndx);

  return 1;
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
    readValue = analogRead(DOOR_BELTSENSOR);
    if (readValue > maxValue)
      maxValue = readValue;
    if (readValue < minValue)
      minValue = readValue;
  }

  // Subtract min from max
  result = ((maxValue - minValue) * 5.0) / 1024.0;

  return result;
}

uint8_t portoHelper(Message *message)
{
  int working = digitalRead(DOOR_BUTTOM);
  //For Manual Work
  if (working)
  {
    //digitalWrite(LED_BUILTIN, HIGH);

    Voltage = getVPP();
    VRMS = (Voltage / 2.0) * 0.707;
    AmpsRMS = (VRMS * 1000) / mVperAmp;

    if (AmpsRMS < 1)
    {
      message->from = myAddress;
      message->to = PCADDR;
      message->command = GET;
      message->GPIOaddr = DOOR_BELTSENSOR;
      message->value = 1;
      message->available = 1;

      delay(2000);
      digitalWrite(DOOR_RELAY, HIGH);
      delay(5000);
    }
    digitalWrite(DOOR_RELAY, LOW);
  }
  else
  {
    //digitalWrite(LED_BUILTIN, LOW);
  }
  return 1;
}

uint8_t applyGPIO(Message *message)
{
  if (gpioports[message->GPIOaddr].Mode == INVALID_PORT)
    return 0;

  switch (message->command)
  {
  case SET:
    digitalWrite(message->GPIOaddr, message->value);
    gpioports[message->GPIOaddr].value = message->value;
    message->to = PCADDR;
    message->from = myAddress;
    message->value = digitalRead(message->GPIOaddr);
    break;

  case GET:
    message->to = PCADDR;
    message->from = myAddress;
    message->value = digitalRead(message->GPIOaddr);
    break;

  case CONFIG:
    pinMode(message->GPIOaddr, message->value);
    message->to = PCADDR;
    message->from = myAddress;
    break;

  default:
    break;
  }
  return 1;
}

uint8_t prossesGPIOInput(Message *message)
{
  for (uint8_t i = 0; i < NUMBEROFGPIOS; i++)
  {
    if (gpioports[i].Mode == INPUT)
    {
      int value = digitalRead(i);
      delay(3);
      int valuea = digitalRead(i);
      if (value != valuea)
        continue;
      if (value != gpioports[i].value)
      {
        gpioports[i].value = value;
        gpioports[i].changed = 1;
        gpioChanged = 1;

        message->from = myAddress;
        message->to = PCADDR;
        message->command = GET;
        message->GPIOaddr = i;
        message->value = value;
        message->available = 1;

        return 1;
      }
    }
  }
  return 0;
}

//LOOP

uint8_t checkInputs(Message *message)
{

  if (Serial.available())
    return checkSerialData(message);

  if (rs485Channel.update())
    return processRS48Message(message);

  if (prossesGPIOInput(message))
    return 1;

  //Looping indefinitly
  return portoHelper(message);
}

//message
// to me - gpio
// to
uint8_t setOutputs(Message message)
{

  //Set by message
  if (message.available)
  {
    if (message.to == myAddress)
      applyGPIO(&message);
    if (message.to == PCADDR && myAddress == MASTERADDR)
      sendSerialData(message);
    else if ((message.to == PCADDR && myAddress != MASTERADDR) ||
             (message.from == PCADDR && myAddress == MASTERADDR))
      sendRS485Message(message);

    gpioChanged = 0;
  }
  return 1;
}

// the loop function runs over and over again forever

void loop()
{
  // Create message
  Message message;
  //Check Serial connection for data (send always)
  message.available = 0;
  // Serial.println(message.address);

  cleanMessage(&message);
  //Check input data
  checkInputs(&message);
  //Output created data
  setOutputs(message);
} //end loop