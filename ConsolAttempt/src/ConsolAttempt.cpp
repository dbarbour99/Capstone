/* 
 * Project Consilidated fingerprint
 * Author: David Barbour
 * Date: 3/27/24
 */

#include "Particle.h"
#include "Adafruit_Fingerprint.h"
#include "IoTTimer.h"
#include "Button.h"
#include "Neopixel.h"
#include "Colors.h"

const int green_led_pin = D0;
const int red_led_pin = D1;
const int BUTTONPIN=D3;
const int NEOPIN=D2;
const int PIXELCOUNT = 12;

//fingerprint reader
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
Adafruit_NeoPixel pixel (PIXELCOUNT,SPI1,WS2812B);

uint8_t getFingerprintEnroll(uint8_t id);
int getFingerprintIDez();
uint8_t getFingerprintID();
int nextFree();
void PixelFill(int startPixel, int endPixel, int theColor);

uint8_t commID = 2;

IoTTimer oneSecTimer;
bool setupMode = false;
Button toggleButton(BUTTONPIN,false);


SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  pixel.begin();
  pixel.setBrightness(22);
  pixel.show();

  //start the serial monitor
  Serial.begin(9600);
  waitFor (Serial.isConnected,10000);

  // set the data rate for the sensor serial port
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }

  Serial.println("Waiting for valid finger...");
  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  digitalWrite(red_led_pin, 0);
  digitalWrite(green_led_pin, 0);

  PixelFill(0,PIXELCOUNT-1,black);

}


void loop() {
  uint8_t theAnswer;
  int nextFreeNum;
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
        break;
      
      default:
        break;
      }
      setupMode=false;

  }else{
    theAnswer=getFingerprintID();
    switch (theAnswer)
    {
    case FINGERPRINT_OK:
      digitalWrite(green_led_pin,1);
      digitalWrite(red_led_pin,0);
      PixelFill(0,PIXELCOUNT,green);
      break;
    
    case FINGERPRINT_NOTFOUND:
      digitalWrite(green_led_pin,0);
      digitalWrite(red_led_pin,1);
      PixelFill(0,PIXELCOUNT,yellow);

    default:
      break;
    }
  }
}

int nextFree()
{
  //finds the next free fingerprint id location
  int i; int retVal;
  bool found=false;
  for(i=0x0000; i<0x00A2;i++)
  {
    if(EEPROM.read(i)!=123)
    {
      found=true;
      retVal= i;
      break;
    }
    
    if(found==false){retVal=-1;}
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
  digitalWrite(green_led_pin, 1);
  digitalWrite(red_led_pin, 0);
  delay(6000);
  digitalWrite(green_led_pin, 0);
  digitalWrite(red_led_pin, 1);
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

void PixelFill(int startPixel, int endPixel, int theColor)
{
  int theLoop;
  for (theLoop=startPixel;theLoop<=endPixel;theLoop++)
  {
    pixel.setPixelColor(theLoop,theColor);
    delay(50);
  }
  pixel.show();
}