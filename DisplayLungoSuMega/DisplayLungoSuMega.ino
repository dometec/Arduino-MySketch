/*
....

For serial comunication os UDP Message input:
Send:
"r" for read internal humidity (ih), internal temperature (it), external humidity (eh), external temperature (et)
"sYXXXX" for send Y times XXXX code from IR Led
"dMessage to display" for display a message
"l123.123.123" show rgb colot in led

for SONY RM-U303:
for 3 time if not specified.
hex//int
Mute	281 //641 
VolUP	481 //1153 almeno per 2 volte, meglio 5 volte...
VolDown	C81 //3201 almeno per 2 volte, meglio 5 volte...

Standby	F41 //3905

Tuner	841 //2113
CD	A41 //2626
Video	441 //1089
5.1Ch	4E1 //1249
MD	961 //2401
TV	561 //1377

BassBos	5909 //22793

In Tuner Mode
  Ch Up	96  //150
  Ch Dw 896 //2198

es: s32113 for switch on Tuner

*/

#include <stdlib.h>
#include <Timer.h>
#include <LiquidCrystal.h>
#include <dht.h>
#include <IRremote.h>
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

#define DHT22_PIN1  34
#define DHT22_PIN2  35
#define RED_PIN     46
#define GREEN_PIN   44
#define BLUE_PIN    45
#define LED_PIN     9

#define RESTORE_DISPLAY_TEMP_SEC  5
#define RESTORE_LED_SEC           2

// MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 177);
unsigned int localPort = 20118;      // local port to listen on

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetServer server(localPort);

dht DHT;
Timer timer;
IRsend irsend;
LiquidCrystal lcd(37, 39, 41, 47, 43, 49, 48, 29, 28, 31, 30);

int count_temp = 2;
int count_led = 2;

float extTem = 0;
float extHum = 0;
float intTem = 0;
float intHum = 0;

String sinput = ""; //Serial input buffer
String ninput = ""; //Nerwork input buffer
char tmpbuff[100];

void setup() {

  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  server.begin();
  
  Serial.begin(9600); 
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);   
  pinMode(LED_PIN, OUTPUT);   
  digitalWrite(LED_PIN, LOW);
  
  analogWrite(RED_PIN, 255);
  analogWrite(GREEN_PIN, 255);
  analogWrite(BLUE_PIN, 255);
    
  // set up the LCD's number of columns and rows: 
  lcd.begin(40, 1);
  
  doUpdateTemp();
  timer.every(5, takeUDPReading);
  timer.every(5, takeSerialReading);
  timer.every(1000, doUpdateTemp);
  timer.every(1000, doRestoreLed);

}

void loop() {
  timer.update();
}

void takeSerialReading() {
   
  while(Serial.available()) {
    
      char character = Serial.read();
      
      if (character == '\r' || character == '\n' || character == '\0') {
        Serial.println(executeCommand(sinput));
        sinput = "";
      } else {
        sinput.concat(character);
      }
      
  }
  
}

void takeUDPReading() {

  EthernetClient client = server.available();
  
  if (client) {
  
    while (client.connected()) {
      if (client.available()) {
        char character = client.read();
        if (character == '\r' || character == '\n' || character == '\0') {
          client.println(executeCommand(ninput));
          delay(100);
          client.stop();
          ninput = "";
        } else {
          ninput.concat(character);
        }
      }
    }
    
  }
  
}


void doUpdateTemp() {
 
  count_temp--;
  
  if (count_temp > 0)
    return;
  
  count_temp = RESTORE_DISPLAY_TEMP_SEC;
  
  if (DHT.read22(DHT22_PIN1) == DHTLIB_OK) {
    extTem = DHT.temperature;
    extHum = DHT.humidity;
  }

  if (DHT.read22(DHT22_PIN2) == DHTLIB_OK) {
    intTem = DHT.temperature;
    intHum = DHT.humidity;
  }

  lcd.home();
  lcd.print("Out: ");  
  lcd.print(extTem, 1);
  lcd.print((char) 223);
  lcd.print("C ");
  lcd.print(extHum, 1);
  lcd.print("%");
  lcd.print("     Int: ");
  lcd.print(intTem, 1);
  lcd.print((char) 223);
  lcd.print("C ");
  lcd.print(intHum, 1);
  lcd.print("%");
  lcd.print(" ");
}

void doRestoreLed() {
 
  count_led--;
  
  if (count_led > 0)
    return;
  
  count_led = RESTORE_LED_SEC;
  
  analogWrite(RED_PIN, random(254, 256));
  analogWrite(GREEN_PIN, random(254, 256));
  analogWrite(BLUE_PIN, random(254, 256));  
  
}

char* executeCommand(String command) {
  
  if (command.length() < 1)
    return "invalid command";
  
  if (command.charAt(0) == 'r') 
    return readSensorData();    
  if (command.charAt(0) == 's') 
    return sendIRCode(command);
  if (command.charAt(0) == 'd')
    return showOnDisplay(command);
  if (command.charAt(0) == 'l')
    return showColorLed(command);
    
  return "";      
}

char* sendIRCode(String command) {
  
  if (command.length() < 5)
    return "invalid code";

  if (command.charAt(0) == 's') {
    
    int cicle = command.charAt(1) - '0';
    command.substring(2).toCharArray(tmpbuff, 5);
    int code = atoi(tmpbuff);

    for (int i = 0; i < cicle; i++) {
      irsend.sendSony(code, 12);
      delay(24);
    }
    return "send";
  }

  return "not understood";  

}

char* readSensorData() {
 
  sprintf(tmpbuff, "ih:%d.%d\nit:%d.%d\neh:%d.%d\net:%d.%d", 
    (int) intHum, 
    ((int) (intHum * 10) - ((int) intHum * 10)),
    (int) intTem,
    ((int) (intTem * 10) - ((int) intTem * 10)),
    (int) extHum,
    ((int) (extHum * 10) - ((int) extHum * 10)),
    (int) extTem,
    ((int) (extTem * 10) - ((int) extTem * 10)));
  
  
  return tmpbuff;
  
}

char* showOnDisplay(String command) {
  
  count_temp = RESTORE_DISPLAY_TEMP_SEC;
  
  lcd.clear();
  lcd.home();
  lcd.print(command.substring(1));
  
  return "show";
  
}

char* showColorLed(String command) {
  
  count_led = RESTORE_LED_SEC;
   
  int p1 = command.indexOf('.');
  int p2 = command.lastIndexOf('.');
    
  if (p1 == -1 || p2 == -1)
    return "error dot";
    
  command.substring(1, p1).toCharArray(tmpbuff, 5);
  int r = atoi(tmpbuff);

  command.substring(p1+1, p2).toCharArray(tmpbuff, 5);
  int g = atoi(tmpbuff);

  command.substring(p2+1).toCharArray(tmpbuff, 5);
  int b = atoi(tmpbuff);

  analogWrite(RED_PIN, 255 - r);
  analogWrite(GREEN_PIN, 255 - g);
  analogWrite(BLUE_PIN, 255 - b);  
 
  return "show";
 
}

