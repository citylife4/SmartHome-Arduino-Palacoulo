#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//RS485 Defines

// the data we broadcast to each other device
struct
{
  byte address;
  byte switches [10];
}  message;

const unsigned long RS485_BAUD_RATE = 9600;
const float TIME_PER_BYTE = 1.0 / (RS485_BAUD_RATE / 10.0);  // seconds per sending one byt
// software serial pins
const byte RO_PIN = 8;
const byte DI_PIN = 9;
// transmit enable
const byte XMIT_ENABLE_PIN = 10;


byte nextAddress;
unsigned long lastMessageTime;
unsigned long lastCommsTime;
unsigned long randomTime;
byte inByte = 0;         // incoming serial byte

SoftwareSerial rs485 ( RO_PIN, DI_PIN);  // receive pin, transmit pin

// from EEPROM
byte myAddress;        // who we are
byte numberOfDevices;  // maximum devices on the bus

// callbacks for the non-blocking RS485 library
size_t fWrite (const byte what) {
  rs485.write (what);
}

int fAvailable () {
  return rs485.available ();
}

int fRead () {
  return rs485.read ();
}

// RS485 library instance
RS485 rs485Channel (fRead, fAvailable, fWrite, 20);

// action pins (demo)
const int REED_PIN = 2; // Pin connected to reed switch
const int LED_PIN = 13; // LED pin - active-high
const byte ENABLE_PIN = 4;
String inString = "";

//Serial Connection Defines
const unsigned long BAUD_RATE = 9600;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(BAUD_RATE);

  // debugging prints
  Serial.println ();
  Serial.println (F("Commencing"));
  myAddress = EEPROM.read (0);
  Serial.print (F("My address is "));
  Serial.println (int (myAddress));
  numberOfDevices = EEPROM.read (1);
  Serial.print (F("Max address is "));
  Serial.println (int (numberOfDevices));

  if (myAddress >= numberOfDevices)
    Serial.print (F("** WARNING ** - device number is out of range, will not be detected."));

  // software serial for talking to other devices
  rs485.begin (RS485_BAUD_RATE);
  // initialize the RS485 library
  rs485Channel.begin ();

  // set up various pins
  pinMode (XMIT_ENABLE_PIN, OUTPUT);

  // demo action pins
  pinMode (ENABLE_PIN, OUTPUT);
  // Since the other end of the reed switch is connected to ground, we need
  // to pull-up the reed switch pin internally.
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite (LED_PIN, HIGH);
}

// Here to process an incoming message
void processSerialData() {
  // handle the incoming message, depending on who it is from and the data in it
  // we cannot receive a message from ourself
  // someone must have given two devices the same address
  if (message.address == myAddress)
  {
    Serial.println ("Problems");
    //digitalWrite (ERROR_PIN, HIGH);
    //while (true)
    //  { }  // give up
  }
  // make our LED match the switch of the previous device in sequence

  Serial.println (int(message.switches [0]));
  Serial.println (message.address);
  digitalWrite (LED_PIN, message.switches [0]);
} // end of processMessage

// Here to send our own message
void sendRS485Message ()
{
  memset (&message, 0, sizeof message);
  message.address = myAddress;
  int inChar = Serial.read();
  if (isDigit(inChar)) {
    // convert the incoming byte to a char and add it to the string:
    inString += (char)inChar;
  }
  message.switches[0] = (char) inString.toInt();
  Serial.write(inString.toInt());
  inString = "";

  digitalWrite (XMIT_ENABLE_PIN, HIGH);  // enable sending
  rs485Channel.sendMsg ((byte *) &message, sizeof message);
  digitalWrite (XMIT_ENABLE_PIN, LOW);  // disable sending

}  // end of sendMessage

// the loop function runs over and over again forever
void loop() {
  //Check Serial connection for data
  while (Serial.available() > 0) {
    sendRS485Message();
  } //end serial.available
  //Check RS connection for data
  if (rs485Channel.update ()) {
    memset (&message, 0, sizeof message);
    int len = rs485Channel.getLength ();
    if (len > sizeof message)
      len = sizeof message;
    Serial.write("bla");
    Serial.write(message.switches[0]);
    memcpy (&message, rs485Channel.getData (), len);
    lastMessageTime = micros ();
    processSerialData ();
  }  // end of message completely received
  //Check outputs from GPIOs

  //Apply commands

} //end loop
