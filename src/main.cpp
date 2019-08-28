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

  // debugging prints
  //Serial.println();
  //Serial.println(F("Commencing"));
  myAddress = EEPROM.read(0);
  //Serial.print(F("My address is "));
  //Serial.println(int(myAddress));
  numberOfDevices = EEPROM.read(1);
  //Serial.print(F("Max address is "));
  //Serial.println(int(numberOfDevices));

  //if (myAddress >= numberOfDevices)
  //  Serial.print(F("** WARNING ** - device number is out of range, will not be detected."));

  // software serial for talking to other devices
  rs485.begin(RS485_BAUD_RATE);
  // initialize the RS485 library
  rs485Channel.begin();

  // set up various pins
  pinMode(XMIT_ENABLE_PIN, OUTPUT);

  // demo action pins
  pinMode(ENABLE_PIN, OUTPUT);
  // Since the other end of the reed switch is connected to ground, we need
  // to pull-up the reed switch pin internally.
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
}

// Here to process an incoming message
void processRS48Message(Message *message)
{

  // make our LED match the switch of the previous device in sequence
  memcpy(message, rs485Channel.getData(), rs485Channel.getLength());
  // handle the incoming message, depending on who it is from and the data in it
  // we cannot receive a message from ourself
  // someone must have given two devices the same address
  //if (message->address == myAddress)
  //{
    //Serial.println("Problems");
    //digitalWrite (ERROR_PIN, HIGH);
    //while (true)
    //  { }  // give up
  //}

  //Serial.write("address");
  //Serial.println(message->address);
  //Serial.write("command");
  //Serial.println(int(message->command));
  //Serial.write("info");
  //Serial.println(int(message->info));

  digitalWrite(LED_PIN, message->address);
} // end of processMessage

void sendSerialData(Message message)
{
  Serial.print(message.address);
  Serial.print("_");
  Serial.print(message.command);
  Serial.print("_");
  Serial.println(message.info);
}

// Here to send our own message
void sendRS485Message(Message message)
{

  digitalWrite(XMIT_ENABLE_PIN, HIGH); // enable sending
  rs485Channel.sendMsg((byte *)&message, sizeof message);
  digitalWrite(XMIT_ENABLE_PIN, LOW); // disable sending

} // end of sendMessage

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

  //Serial.write("address ");
  //Serial.println(int(message->address));
  //Serial.write("command ");
  //Serial.println(message->command);
  //Serial.write("info ");
  //Serial.println(int(message->info));

  inString = "";
  //settign available to 1
  message->available = 1;
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