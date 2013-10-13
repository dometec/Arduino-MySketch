#include <stdarg.h>
#include <LiquidCrystal.h>
#include <max6675.h>
#include <Wire.h>

// Pins
#define relePIN    6
#define buzzerPIN  4
#define thermoDO  14
#define thermoCS   5
#define thermoCLK 15

// Button ID
#define btnNONE    0
#define btnON      2
#define btnOFF     1
#define btnCPlus   4
#define btnCMin   16
#define btnTPlus   8
#define BtnTMin   32

#define owenOFF 0
#define owenProgramming 1
#define owenHeating 2
#define owenON 3

#define defProgrammNumber  3

#define maxSecInProgrammMode 5

//Default programm
int currentProgramm = 0;
int defProgramm[defProgrammNumber][2] = {{1800, 180}, {1800, 200}, {1200, 80} };

int currentSecLeft = 0;
int currentTemp = 0;
int targetTemp = 0;

unsigned long endMillSecInProgrammMode = 0;
int lastSec = 0;

// Status Owen: 0 OFF, 1 Programming, 2 3 ON
int owenStatus = 0;

char bufDispl[16];
int countTick = 0;

String serialStart = String("S");
String serialSeparator = String(";");

// Initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

void setup(void) {
  
  pinMode(relePIN, OUTPUT);
  pinMode(buzzerPIN, OUTPUT);
  buzz(buzzerPIN, 4186, 100);
  
  Serial1.begin(9600); 
  lcd.begin(16, 2);
  Wire.begin();  
  
  Serial1.println("AT+BAUD4");
  delay(100);
  while (Serial1.available() > 0)
    Serial1.read();
  
  Serial1.println("AT+NAMEFornoCristina");
  delay(100);
  while (Serial1.available() > 0)
    Serial1.read();  
    
  Serial1.println("AT+PIN0000");
  delay(100);
  while (Serial1.available() > 0)
    Serial1.read();
    
  updateDisplay();
  
}

void loop(void) {
  
  countTick++;
  
  //read temp (una volta al secondo circa)
  if (countTick %100 == 0)
    currentTemp = (int) thermocouple.readCelsius();
  
  //----- ACTION -------
  int btn = getButton();
  if (btn == 0 && Serial1.available() > 0) {
    int fromSerial = Serial1.read();
    if (fromSerial > 0)
      btn = fromSerial;
  }
  
  if (btn == btnON && (owenStatus == owenOFF || owenStatus == owenProgramming)) {
    
    owenStatus = owenProgramming;
    endMillSecInProgrammMode = millis() + (maxSecInProgrammMode * 1000);
    
    if (currentProgramm == defProgrammNumber) {
      currentProgramm = 0;
    }
    
    currentSecLeft = defProgramm[currentProgramm][0];
    targetTemp = defProgramm[currentProgramm][1];
        
    currentProgramm++;
    buzz(buzzerPIN, 4186, 50);

    updateDisplay();    
    delay(500);
  }
  
  if (btn == btnOFF && owenStatus != owenOFF) {
    owenStatus = owenOFF;
    updateDisplay();
    digitalWrite(relePIN,LOW);
    buzz(buzzerPIN, 4186, 100);
    delay(100);
    buzz(buzzerPIN, 4186, 100);
    delay(100);
    buzz(buzzerPIN, 4186, 100);
  }
  
  if (owenStatus == owenProgramming && millis() > endMillSecInProgrammMode) {
    owenStatus = owenHeating;
    updateDisplay();
    buzz(buzzerPIN, 4186, 100);
  }
  
  if (btn == btnCPlus && owenStatus != owenOFF) {
    targetTemp += 10;
    endMillSecInProgrammMode = millis() + (maxSecInProgrammMode * 1000);
    updateDisplay();
    buzz(buzzerPIN, 4186, 100);
    delay(500);
  }

  if (btn == btnCMin && owenStatus != owenOFF && targetTemp >10) {
    targetTemp -= 10;
    endMillSecInProgrammMode = millis() + (maxSecInProgrammMode * 1000);
    updateDisplay();
    buzz(buzzerPIN, 4186, 100);
    delay(500);
  }
  
  if (btn == btnTPlus && owenStatus != owenOFF) {
    currentSecLeft += 60;
    endMillSecInProgrammMode = millis() + (maxSecInProgrammMode * 1000);
    updateDisplay();
    buzz(buzzerPIN, 4186, 100);
    delay(500);
  }

  if (btn == BtnTMin && owenStatus != owenOFF && currentSecLeft > 61) {
    currentSecLeft -= 60;
    endMillSecInProgrammMode = millis() + (maxSecInProgrammMode * 1000);
    updateDisplay();
    buzz(buzzerPIN, 4186, 100);
    delay(500);
  }
  
  if (owenStatus == owenHeating && currentTemp >= targetTemp) {
    owenStatus = owenON;
    updateDisplay();
    buzz(buzzerPIN, 4186, 1500);
  }
  
  if (currentTemp >= targetTemp || owenStatus == owenOFF) 
    digitalWrite(relePIN, LOW);
  else
    digitalWrite(relePIN, HIGH);
    
  int nowSec = millis() / 1000;
  if (owenStatus == owenON && lastSec != nowSec) {
    currentSecLeft--;
    lastSec = millis() / 1000;
  }
    
  if (currentSecLeft <= 0 && owenStatus != owenOFF) {
    owenStatus = owenOFF;
    buzz(buzzerPIN, 4186, 100);
    delay(100);
    buzz(buzzerPIN, 4186, 100);
    delay(100);
    buzz(buzzerPIN, 4186, 1000);
  }
  
  //Update display (una volta al secondo circa)
  if (countTick %50 == 0)
    updateDisplay();
  
  //Per debug visivo
  delay(10);
  buzz(13, 4186, 10);
  
}
 
int getButton(void) {
  Wire.beginTransmission(0x1B); // transmit to device 
  Wire.write(3); // want to read key status // set pointer
  Wire.endTransmission();      // stop transmitting
  Wire.requestFrom(0x1B, 1);    // request 1 byte from slave device
  return Wire.read();
}

void buzz(int targetPin, long frequency, long length) {
  
  long delayValue = 1000000/frequency/2;
  long numCycles = frequency * length/ 1000;
  
  for (long i=0; i < numCycles; i++) {
    digitalWrite(targetPin, HIGH);
    delayMicroseconds(delayValue);
    digitalWrite(targetPin, LOW);
    delayMicroseconds(delayValue);
  }
  
}

void updateDisplay(void) {
  
  lcd.clear();
  lcd.setCursor(0, 0);
    
  if (owenStatus == owenOFF) {
    lcd.print("Forno spento");    
    sprintf(bufDispl, "adesso a %dC", currentTemp);
  }
  
  if (owenStatus == owenON) {
    lcd.print("Forno acceso");    
    int mins = currentSecLeft / 60;
    int secs = currentSecLeft % 60;
    sprintf(bufDispl, "%02d:%02d %dC(%d)", mins, secs, currentTemp, targetTemp);
  }
  
  if (owenStatus == owenHeating) {
    lcd.print("Riscaldamento");    
    int mins = currentSecLeft / 60;
    int secs = currentSecLeft % 60;
    sprintf(bufDispl, "%02d:%02d %dC(%d)", mins, secs, currentTemp, targetTemp);
  }

  if (owenStatus == owenProgramming) {  
    lcd.print("Programma");    
    int mins = currentSecLeft / 60;
    int secs = currentSecLeft % 60;
    sprintf(bufDispl, "%02d:%02d a %dC", mins, secs, targetTemp);    
  }

  lcd.setCursor(0, 1);   
  lcd.print(bufDispl);
  Serial1.println(serialStart + owenStatus + serialSeparator + bufDispl);
  
}
