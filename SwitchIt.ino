#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"

#define FONA_RX  9
#define FONA_TX  8
#define FONA_RST 4
#define FONA_RI  7


#define SOCKET_1 A2
#define SOCKET_2 A3

#define LED 13

#define OFF true
#define ON false

#define BUSYWAIT 5000  // milliseconds

/*
 ***************************
 * This line is used for debugging to the serial monitor
 * if you leave these in debug mode, it does slow down your program when 
 * not connected to USB
 *****************************
 */
//#define Sprintln(a) (Serial.println(a)) //defined on
#define Sprintln(a) //defined off
//#define Sprint(a) (Serial.print(a)) //defined on
#define Sprint(a)  //defined off


// this is a large buffer for replies
char replybuffer[255];
int val=0; // this is just a status buffer.



// or comment this out & use a hardware serial port like Serial1 (see below)
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

boolean fonainit(void) {
  Sprintln(F("Initializing....(May take 3 seconds)"));
   changeState(OFF,800);

  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
   
  // make it slow so its easy to read!
  fonaSS.begin(4800); // if you're using software serial
  //Serial1.begin(4800); // if you're using hardware serial

  // See if the FONA is responding
  if (! fona.begin(fonaSS)) {           // can also try fona.begin(Serial1) 
    Sprintln(F("Couldn't find FONA"));
    return false;
  }
  Sprintln(F("FONA is OK"));
  return true;
  
}

void setup() {
  //while (!Serial);  // useful for Leonardo/Micro, remove in production!

  // set LED output for debugging
  pinMode(LED, OUTPUT);
  
  // set up relay pins, but turn them off
  pinMode(SOCKET_1, OUTPUT);
  digitalWrite(SOCKET_1, LOW);
  pinMode(SOCKET_2, OUTPUT);
  digitalWrite(SOCKET_2, LOW);  
  
  Serial.begin(115200);
  Sprintln(F("FONA basic test"));

  while (! fonainit()) {
    delay(5000);
 
  
  }
  
  // Print SIM card IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Sprint("SIM card IMEI: "); Sprintln(imei);
  }
// Optionally configure a GPRS APN, username, and password.
// You might need to do this to access your network's GPRS/data
// network. Contact your provider for the exact APN, username,
// and password values. Username and password are optional and
// can be removed, but APN is required.
//fona.setGPRSNetworkSettings(F("your APN"), F("your username"), F("your password"));
  
  
  pinMode(FONA_RI, INPUT);
  digitalWrite(FONA_RI, HIGH); // turn on pullup on RI
  // turn on RI pin change on incoming SMS!
  fona.sendCheckReply(F("AT+CFGRI=1"), F("OK"));
}


int8_t lastsmsnum = 0;

void loop() {
   digitalWrite(LED, HIGH);
   delay(10);
   digitalWrite(LED, LOW);
       
  while (fona.getNetworkStatus() != 1) {
    Sprintln("Waiting for cell connection");
    delay(2000);
  }
  
  
  // this is a 'busy wait' loop, we check if the interrupt
  // pin went low, and after BUSYWAIT milliseconds break out to check
  // manually for SMS's and connection status
  // This would be a good place to 'sleep'
  for (uint16_t i=0; i<BUSYWAIT; i++) {
     if (! digitalRead(FONA_RI)) {
        // RI pin went low, SMS received?
        Sprintln(F("RI went low SMS Received"));
        break;
     } 
     delay(1);
  }
  
  int8_t smsnum = fona.getNumSMS();
  if (smsnum < 0) {
    Sprintln(F("No SMS on Card"));
    return;
  } else {
    Sprint(smsnum); 
    Sprintln(F(" SMS's on SIM card!"));
  }
  
  if (smsnum == 0) return;

  // there's an SMS!
  
  uint8_t n = 1; 
  while (true) {
     uint16_t smslen;
     char sender[25];
     
     Sprint(F("\n\rReading SMS #")); Sprintln(n);
     uint8_t len = fona.readSMS(n, replybuffer, 250, &smslen); // pass in buffer and max len!
     
     // added to truncate spaces that come over in the text message.  it causes werid things to happen
     char * p = strchr (replybuffer, ' '); //search for a space in the SMS Message
     if (p) //if found, trucate at space
     *p = 0;
     
     // if the length is zero, its a special case where the index number is higher
     // so increase the max we'll look at!
     if (len == 0) {
        Sprintln(F("[empty slot]"));
        n++;
        continue;
     }
     if (! fona.getSMSSender(n, sender, sizeof(sender))) {
       // failed to get the sender?
       sender[0] = 0;
     }
     
     Sprint(F("***** SMS #")); Sprint(n);
     Sprint(" ("); Sprint(len); Sprintln(F(") bytes *****"));
     Sprintln(replybuffer);
     Sprint(F("From: ")); Sprintln(sender);
     Sprintln(F("*****"));
     
     if (strcasecmp(replybuffer, "Off") == 0) {
       // Turn on socket!
       digitalWrite(LED, HIGH);
       digitalWrite(LED, LOW);
       digitalWrite(LED, HIGH);
       changeState(OFF, 800);
       sendStatus(sender);
       digitalWrite(LED, LOW);
     }
     if (strcasecmp(replybuffer, "On") == 0) {
       // close the door!
       digitalWrite(LED, HIGH);
       changeState(ON, 800);
       sendStatus(sender);
       digitalWrite(LED, LOW);
     }
     if (strcasecmp(replybuffer, "Status")==0){
      sendStatus(sender);
      
     }
     
     delay(3000);
     break;
  }  
  fona.deleteSMS(n);
 
  delay(1000); 
}

void sendStatus(char *sender){
      Sprint("****sendStatus"); Sprint(sender);
      //use this to send status a message.
      val=digitalRead(SOCKET_1);
      char status_text[32] ="Switch is: ";
      if(val==0){strcat(status_text,"on");
      }else{strcat(status_text,"off");}
      Sprint(F("Status info:")); Sprintln(status_text);
      if (!fona.sendSMS(sender, status_text)) {
        Sprintln(F("Failed"));
      } else {
        Sprintln(F("Sent!"));
      }
}

void sendText(char number[32], char message[255]){
     if(sizeof(message) > 0){
        if(!fona.sendSMS(number,message)){
         Sprintln(F("failed to send"));
        } else {
          Sprintln(F("Sent Message"));
        }
     } else {
        Sprintln(F("no message in body"));
     }
}

void changeState(boolean direction, uint16_t t) {
  if (direction) {
    digitalWrite(SOCKET_1, HIGH);
    digitalWrite(SOCKET_2, HIGH);
  } else {
    digitalWrite(SOCKET_2, LOW);
    digitalWrite(SOCKET_1, LOW);  
  }
 
}
