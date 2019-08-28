#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//RS485 Defines

// the data we broadcast to each other device changes
class Message
{

public:
  int address;
  byte command;
  byte info;
  byte available;
};

Message message;

const unsigned long RS485_BAUD_RATE = 9600;
const float TIME_PER_BYTE = 1.0 / (RS485_BAUD_RATE / 10.0); // seconds per sending one byt
// software serial pins
const byte RO_PIN = 8;
const byte DI_PIN = 9;
// transmit enable
const byte XMIT_ENABLE_PIN = 10;

byte nextAddress;
unsigned long lastMessageTime;
unsigned long lastCommsTime;
unsigned long randomTime;
byte inByte = 0; // incoming serial byte

SoftwareSerial rs485(RO_PIN, DI_PIN); // receive pin, transmit pin

//For Porto..
const int sensorIn = A7;
int mVperAmp = 100; // use 100 for 20A Module and 66 for 30A Module
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

int BUTTON_IN = 3;
int BUTTON_OUT = 2;

int working = 1;
int received ;

// from EEPROM
byte myAddress;       // who we are
byte numberOfDevices; // maximum devices on the bus

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
RS485 rs485Channel(fRead, fAvailable, fWrite, 20);

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
  // initialize serial communication at 9600 bits per second:
  Serial.begin(BAUD_RATE);

  myAddress = EEPROM.read(0);
  numberOfDevices = EEPROM.read(1);

  rs485.begin(RS485_BAUD_RATE);
  // initialize the RS485 library
  rs485Channel.begin();

  // set up various pins
  //pinMode(XMIT_ENABLE_PIN, OUTPUT);

  // demo action pins
  //pinMode(ENABLE_PIN, OUTPUT);
  // Since the other end of the reed switch is connected to ground, we need
  // to pull-up the reed switch pin internally.

  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, HIGH);

    pinMode (ENABLE_PIN, OUTPUT);  // driver output enable
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
  memcpy(message, rs485Channel.getData(), rs485Channel.getLength());
  digitalWrite(LED_PIN, message->address);
} // end of processMessage

// Here to send our own message
void sendRS485Message(Message message)
{

  digitalWrite(XMIT_ENABLE_PIN, HIGH); // enable sending
  rs485Channel.sendMsg((byte *)&message, sizeof message);
  digitalWrite(XMIT_ENABLE_PIN, LOW); // disable sending

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


void checkSerialData(Message *message)
{
  String serialResponse = Serial.readStringUntil(';');

  // Convert from String Object to String.
  char buf[20];
  serialResponse.toCharArray(buf, sizeof(buf));
  char *p = buf;
  char *str;

  //there should be a cleaner way...
  int i = 0;
  while ((str = strtok_r(p, "_", &p)) != NULL)
  {
    switch (i)
    {
    case 0:
      message->address = *str;
      break;
    case 1:
      message->command = *str;
      break;
    case 2:
      message->info = *str;
      break;
    }
    i++;
  }


  inString = "";
  //settign available to 1
  message->available = 1;
}

//
// GPIO
//


float getVPP()
{
  float result;
  
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 100) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue) 
           maxValue = readValue;
       if (readValue < minValue) 
           minValue = readValue;
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 5.0)/1024.0;
      
   return result;
 }

void portoHelper()
{
  working = digitalRead ( BUTTON_IN );
    //For Manual Work
 if (working) {
  digitalWrite(LED_BUILTIN, HIGH); 
  
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707; 
  AmpsRMS = (VRMS * 1000)/mVperAmp;

  
  if( AmpsRMS < 1 ) {
    Serial.write("1_portohelper_opening");
    delay(2000);
    digitalWrite(BUTTON_OUT, HIGH);
    delay(5000);
  } 
  digitalWrite(BUTTON_OUT, LOW);
 } else {
  digitalWrite(LED_BUILTIN, LOW);
 }
 
}


void checkGPIOutput(Message *message)
{

}

void prossesGPIOInput(Message message)
{

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
  //Send Message to serial
  if (message.available)
  {
    sendSerialData(message);
  }

  //Send Message RS485
  if (message.available)
  {
    sendRS485Message(message);
  }

} //end loop