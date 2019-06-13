#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


//Serial Connection Defines
const unsigned long BAUD_RATE = 9600;

const unsigned long RS485_BAUD_RATE = 9600;

unsigned long lastMessageTime;
unsigned long lastCommsTime;
unsigned long randomTime;

// action pins (demo)
const int REED_PIN = 2; // Pin connected to reed switch
const int LED_PIN = 13; // LED pin - active-high

//
const int message_size = 10;

const float TIME_PER_BYTE = 1.0 / (RS485_BAUD_RATE / 10.0);  // seconds per sending one byt
// software serial pins
const byte RO_PIN = 8;
const byte DI_PIN = 9;
// transmit enable
const byte RS485_ENABLE_BYTE = 10;
const byte ENABLE_PIN = 4;

byte inByte = 0;         // incoming serial byte
byte nextAddress;

// from EEPROM
byte myAddress;        // who we are
byte numberOfDevices;  // maximum devices on the bus


// the data we broadcast to each other device
struct
{
  byte address;
  byte switches [message_size];
}  message;


//Create RS485 
SoftwareSerial rs485        ( RO_PIN, DI_PIN);  // receive pin, transmit pin

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
RS485          rs485Channel (fRead, fAvailable, fWrite, 20);


// Here to process an incoming message
void processSerialData() {
  // handle the incoming message, depending on who it is from and the data in it
  // we cannot receive a message from ourself
  // someone must have given two devices the same address
  if (message.address == myAddress)
    Serial.println ("Problems");
    //TODO: We should do somethin here...

  // make our LED match the switch of the previous device in sequence

  int i;
  int myMessage ;
  for (i=0 ; i<message_size; i++){
    myMessage = message.switches [i];
    Serial.print (i);
    Serial.print (": ");
    Serial.print (myMessage);
    Serial.print (" ");
  }
  Serial.println ();

} // end of processMessage

//Where we process the data received on the RS485 Channel
void UpdateRS485Info() {
    memset (&message, 0, sizeof message);
    int len = rs485Channel.getLength ();
    if (len > sizeof message)
      len = sizeof message;
    memcpy (&message, rs485Channel.getData (), len);
    lastMessageTime = micros ();
    processSerialData ();
}

// Here to send our own message
void sendRS485Message ()
{
  //Create a message
  memset (&message, 0, sizeof message);
  
  //Save myAdrress first
  message.address = myAddress;

  int i=0;
  //Check Serial connection for data
  while (Serial.available() && i<message_size) {
    char c = Serial.read();  //gets one byte from serial buffer
    message.switches[i++] = c;
    Serial.print (c);
    delay(2);
  }

  digitalWrite (RS485_ENABLE_BYTE, HIGH);  // enable sending
  rs485Channel.sendMsg ((byte *) &message, sizeof message);
  digitalWrite (RS485_ENABLE_BYTE, LOW);  // disable sending

}  // end of sendMessage

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
  pinMode (RS485_ENABLE_BYTE, OUTPUT);

  // demo action pins
  pinMode (ENABLE_PIN, OUTPUT);
  // Since the other end of the reed switch is connected to ground, we need
  // to pull-up the reed switch pin internally.
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite (LED_PIN, HIGH);
}

// the loop function runs over and over again forever
void loop() {
  
  //Maybe we want this only one address only
  if (Serial.available() > 0 ) {
    sendRS485Message();
  } //end serial.available

  //Check the Bus for data and update GPIOs
  if (rs485Channel.update ()) {
    UpdateRS485Info();
  }  // end of message completely received
  
  //Check outputs from GPIOs

  //Apply commands

} //end loop