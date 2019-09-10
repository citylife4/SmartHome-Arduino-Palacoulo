#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Defines
#define INPUT_SIZE 50
#define NUMBEROFGPIOS 20
#define INVALID_PORT 0x04

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

} ;

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

//State machine
enum State_enum {SET, CONFIG, READ};
//enum Sensors_enum {NONE, SENSOR_RIGHT, SENSOR_LEFT, BOTH};

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

//For now it will only accept one communication at a time
//comand:info (missing reading address)
uint8_t checkSerialData(Message *message)
{
  // Get next command from Serial (add 1 for final 0)
  char input[INPUT_SIZE + 1];
  char delim[] = ":";
  Serial.readBytes(input, INPUT_SIZE);

  message->available = 1;
  // Add the final 0 to end the C string
  message->to=atoi(strtok(input , delim));
  message->command=atoi(strtok(NULL , delim));
  message->GPIOaddr=atoi(strtok(input , delim));
  message->value=atoi(strtok(input , delim));
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
    readValue = analogRead(sensorIn);
    if (readValue > maxValue) maxValue = readValue;
    if (readValue < minValue) minValue = readValue;
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

void applyGPIO(Message message)
{

}

uint8_t prossesGPIOInput(Message * message)
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

        message->to = 0;
        message->command=READ;
        message->value = value;
        message->available=1;

        return 1;
      }
    }
  }
  return 0;
}

uint8_t checkInputs(Message * message) {

  if (Serial.available())
    return checkSerialData(message);

  if (rs485Channel.update())
    return processRS48Message(message);

  return prossesGPIOInput(message);

}

uint8_t setOutputs(Message message) {

  if (message.available )
  {
    if (message.to == myAddress)
    {
      applyGPIO(message);
    } 
    sendSerialData(message);
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

  //Check input data
  checkInputs(&message);

  //Output created data
  setOutputs(message);
} //end loop