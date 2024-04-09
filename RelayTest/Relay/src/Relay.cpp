/* 
 * Project Test Relay
 * Author: David Barbour
 * Date: 4/5/24
 */

#include "Particle.h"
#include "Adafruit_Fingerprint.h"
#include "IoTTimer.h"
#include "Button.h"
#include "Neopixel.h"
#include "Colors.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

//oled screen
  const int OLED_RESET=-1;
  Adafruit_SSD1306 display(OLED_RESET);
  IoTTimer displayTimer;

//current sensor
  int noOfChannel = 0;
  int Addr = 0x2A;
  float current = 0.0;
  int typeOfSensor = 0;
  int maxCurrent = 0;
  byte data[36];

//relay to drive 115/115 volts
  const int RELAYPIN = D14;
  bool OnOff = false;

//fingerprint reader
  const int REDLEDPIN = D2, GREENLEDPIN = D3, FINGERMODEPIN=D4;
  Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
  IoTTimer oneSecTimer;
  bool setupMode = false;
  Button toggleButton(FINGERMODEPIN,false);
  uint8_t getFingerprintID();
  uint8_t getFingerprintEnroll(uint8_t id);
  uint8_t fingerID;
  uint8_t nextFree();
  int getFingerprintIDez();
  Button logOff;
  boolean loggedOn = false;

//Let Device OS manage the connection to the Particle Cloud
  SYSTEM_MODE(SEMI_AUTOMATIC);

//test sub routines
  void testRelay();
  uint8_t testFingerPrint();
  void testCurrentReader();

void setup() {
  //start the serial monitor
    Serial.begin(9600);
    waitFor (Serial.isConnected,10000);

  //start the oled display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    display.clearDisplay();

  //relay for 110/115 current
    pinMode(RELAYPIN,OUTPUT);

  //start the serial monitor
    Serial.begin(9600);
    waitFor (Serial.isConnected,10000);

  //start the fingerprint sensor
    finger.begin(57600);
    if (finger.verifyPassword()) {
      Serial.println("Found fingerprint sensor!");
    } else {
      Serial.println("Did not find fingerprint sensor :(");
      while (1);
    }
    Serial.println("Waiting for valid finger...");
    pinMode(REDLEDPIN, OUTPUT);
    pinMode(GREENLEDPIN, OUTPUT);
    digitalWrite(REDLEDPIN, 0);
    digitalWrite(GREENLEDPIN, 0);

  //start the current reader
    Wire.begin();
    Wire.beginTransmission(Addr);
    Wire.write(0x92);
    Wire.write(0x6A);
    Wire.write(0x02);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0xFE);
    Wire.endTransmission();

  // Request 6 bytes of data
    Wire.requestFrom(Addr, 6);
    if (Wire.available() == 6)
    {
      data[0] = Wire.read();
      data[1] = Wire.read();
      data[2] = Wire.read();
      data[3] = Wire.read();
      data[4] = Wire.read();
      data[5] = Wire.read();
    }
    typeOfSensor = data[0];
    maxCurrent = data[1];
    noOfChannel = data[2];

  // Output data to dashboard
    Serial.printf("Type of Sensor %i \n",typeOfSensor);
    Serial.printf("Max Current: %i \n", maxCurrent);
    Serial.printf("No. of Channels: %i 'n", noOfChannel);
  
  displayTimer.startTimer(50);
}


void loop() {

  if(displayTimer.isTimerReady()==true){
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10,0);
    display.clearDisplay();
    display.printf("Se%cior\n David",164);
    display.display();
    displayTimer.startTimer(2000);
  }

  //testRelay();
  //fingerID = testFingerPrint();
  testCurrentReader();
}

void testCurrentReader()
{

  //turn the relay on or off with the button
  if (toggleButton.isClicked()==true){OnOff = !OnOff;}
  if (OnOff==true){
    digitalWrite(RELAYPIN,HIGH);
  }
  else{
    digitalWrite(RELAYPIN,LOW);
  }

  for (int j = 1; j < noOfChannel + 1; j++)
  {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Command header byte-1
    Wire.write(0x92);
    // Command header byte-2
    Wire.write(0x6A);
    // Command 1
    Wire.write(0x01);
    // Start Channel No.
    Wire.write(j);
    // End Channel No.
    Wire.write(j);
    // Reserved
    Wire.write(0x00);
    // Reserved
    Wire.write(0x00);
    // CheckSum
    Wire.write((0x92 + 0x6A + 0x01 + j + j + 0x00 + 0x00) & 0xFF);
    // Stop I2C Transmission
    Wire.endTransmission();
    delay(500);

    // Request 3 bytes of data
    Wire.requestFrom(Addr, 3);

    // Read 3 bytes of data
    // msb1, msb, lsb
    int msb1 = Wire.read();
    int msb = Wire.read();
    int lsb = Wire.read();
    current = (msb1 * 65536) + (msb * 256) + lsb;

    // Convert the data to ampere
    current = current / 1000;

    // Output data to dashboard
    Serial.printf("Channel: %i \n", j);
    Serial.printf("Current Value: %0.2f \n", current); 

    delay (1000);
  }
}


uint8_t testFingerPrint()
{
  uint8_t theAnswer, nextFreeNum;

  //finger.emptyDatabase();
  if (toggleButton.isClicked()==true){setupMode=!setupMode;}

  if (setupMode==true){
      //This is setup mode
      
      nextFreeNum = nextFree();
      Serial.printf("Setup mode, next free position is %i\n",nextFreeNum);
      finger.deleteModel(nextFreeNum);
      theAnswer = getFingerprintEnroll(nextFreeNum);
      switch (theAnswer)
      {
      case FINGERPRINT_OK:
        EEPROM.write(0x0000+nextFreeNum,123);
        Serial.printf("Enrolled at position %i",nextFreeNum);
        theAnswer = nextFreeNum;
        break;
      
      default:
        theAnswer = 999;
        break;
      }
      setupMode=false;

  }else{
    theAnswer=getFingerprintID();
    switch (theAnswer)
    {
    case FINGERPRINT_OK:
      digitalWrite(GREENLEDPIN,1);
      digitalWrite(REDLEDPIN,0);
      break;
    
    case FINGERPRINT_NOTFOUND:
      digitalWrite(GREENLEDPIN,0);
      digitalWrite(REDLEDPIN,1);

    default:
      break;
    }
  }
  return theAnswer;
}

uint8_t nextFree()
{
  //finds the next free fingerprint id location
  uint8_t i; uint8_t retVal;
  bool found=false;
  for(i=0x0000; i<0x00A2;i++)
  {
    if(EEPROM.read(i)!=123)
    {
      found=true;
      retVal= i;
      break;
    }
    
    if(found==false){retVal=999;}
  }
  return retVal;
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); 
  Serial.println(finger.confidence); 
  return p;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -2;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -3;
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  digitalWrite(GREENLEDPIN, 1);
  digitalWrite(REDLEDPIN, 0);
  delay(6000);
  digitalWrite(GREENLEDPIN, 0);
  digitalWrite(REDLEDPIN, 1);
  return finger.fingerID; 
}

uint8_t getFingerprintEnroll(uint8_t id) {
  uint8_t p = -1;
  Serial.println("Waiting for valid finger to enroll");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  
  // OK converted!
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  return p;
}

void testRelay()
{
  int i;
  bool OnOFF = true;

  for (i=0;i<10;i++){
    if (OnOFF==true){
      digitalWrite(RELAYPIN,HIGH);
    }
    else{
      digitalWrite(RELAYPIN,LOW);
    }
    OnOFF = !OnOFF;
    delay(1000);
  }
}