/* 
 * Project CurrentMaster capstone project
 * Author: David Barbour
 * Date: 4/19/24
 */

#include "Particle.h"
#include "Adafruit_Fingerprint.h"
#include "IoTTimer.h"
#include "Button.h"
#include "Neopixel.h"
#include "Colors.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "application.h"

//if you set this to true, the fingerprint database gets deleted
//and all the EPROM reservation registers are cleared
  bool resetAll = false;

// web hook data for getting user name (and if they're authorized)
  const char *EVENT_NAME = "GetUsername";
  String userSubscription= "hook-response/GetUsername";
  int pFingerID=1, pEquipID=2;
  void subscriptionHandler(const char *event, const char *data);
  void sendLoginRequest(int sFingerID,int sEquipID);

//web hook for writing current data
  const char *EVENT_NAME_ACTIVE = "AddActivity";
  String userSubscription_ACTIVE= "hook-response/AddActivity";
  void subscriptionHandler_ACTIVE(const char *event_active, const char *data_active);
  void sendData(int sFingerID,int sEquipID,int sCurrent,int sType);
  IoTTimer sendDataTimer;
  
//Neo pixel for status
  Adafruit_NeoPixel pixel (1,SPI,WS2812B);
  bool pixelOnOff;
  IoTTimer pixelTimer;

//oled screen
  const int OLED_RESET=-1;
  Adafruit_SSD1306 display(OLED_RESET);
  IoTTimer timerDisplay;
  bool bolDisplay;
  int secDisplay=1;

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
  const int checkForPrintEvery=1000;
  Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
  IoTTimer checkForPrintTimer;
  IoTTimer rejectTimer;
  Button toggleButton(FINGERMODEPIN,false);
  Button logOff (D12,true);

  bool setupMode = false; bool loggedOn = false;
  uint8_t theAnswer; int nextFreeNum;uint8_t fingerID; 

  uint8_t getFingerprintID();
  uint8_t getFingerprintEnroll(uint8_t id);
  uint8_t getFingerID();
  int getNextFree();

//Program flow variables
  const int TIMEZONE = -6;const int BUTTONLOGOFF=D12;const int equipID = 1;
  int theState = 0;int userID=-1;int cumCurrent = 0; int lastCurrent=0;
  String userName="";String startTime="", finishTime="";String TimeOnly;
  Button logOffButton(BUTTONLOGOFF,true);
  IoTTimer displayError; int displayErrorFor = 5000;

  void programLogic();
  void setStatusPixel();
  void displayText();

SYSTEM_MODE(AUTOMATIC);

void setup() {

  //get neopixel ready to display
    pixel.begin();
    pixel.setBrightness(22);
    pixelTimer.startTimer(0);
    setStatusPixel();

  //start the oled display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    display.clearDisplay();
    displayText();

  //start the serial monitor
    Serial.begin(9600);
    waitFor (Serial.isConnected,10000);

  //get current time
    Time.zone (-6);
    Particle.syncTime();

  //web hook
    Particle.subscribe("hook-response/GetUsername", subscriptionHandler, MY_DEVICES);

  //set up the fingerprint reader
    finger.begin(57600);
    if (finger.verifyPassword()) {
      Serial.println("Found fingerprint sensor!");
    } else {
      Serial.println("Did not find fingerprint sensor :(");
      while (1);
    }
    pinMode(REDLEDPIN, OUTPUT);
    pinMode(GREENLEDPIN, OUTPUT);
    digitalWrite(REDLEDPIN, D1);
    digitalWrite(GREENLEDPIN, D0);
    checkForPrintTimer.startTimer(10);

    //initialze the error display timer
    displayError.startTimer(10);

    //delete all the fingerprints and EPROM positions
    if(resetAll==true){
      finger.emptyDatabase();
      for(int i=0x0000; i<0x00A2;i++){
        EEPROM.write(i,0);
      }
      for(int i=0x0000; i<0x0020;i++){
        int theVal = EEPROM.read(i);
        Serial.printf("Loca %0x value %i\n",i,theVal);
      }

      int NextNum = getNextFree();
      Serial.printf("Fingerprint reader reset\n");
      Serial.printf("Next free number %i\n",NextNum);

      delay(20000);
    }
}


void loop() { 

  //set the oled text based on the state
  displayText();

  //set the status neo pixel based on the state
  setStatusPixel();

  //react to the user pushing the logoff button (only if they are logged in)
  if(logOffButton.isClicked()==true && theState==3){theState = 4;}

  //this is the main program logic  
  programLogic();

}

void programLogic()
{
  switch (theState){
      
      case 0: //startup
        //make sure electical is disabled
        digitalWrite(RELAYPIN,LOW);

        //after you're connected to the particle clound, update the state to ready for login
        if(Particle.connected()==true){theState=1;}
        break;
      
      case 1: //waiting for login

        //if the user hit's the setup button, allow them
        //to store more fingerprints
        if(toggleButton.isPressed()==true){
          setupMode= !setupMode;
          Serial.printf("Setupmode = %i\n",setupMode);}

        if (setupMode==true){
          //This is setup mode
          nextFreeNum = getNextFree();
          Serial.printf("Setup mode, next free position is %i\n",nextFreeNum);
          finger.deleteModel(nextFreeNum);
          theAnswer = getFingerprintEnroll(nextFreeNum);
          switch (theAnswer)
          {
          case FINGERPRINT_OK:
            EEPROM.write(0x0000+nextFreeNum,123);
            Serial.printf("Enrolled at position %i\n",nextFreeNum);
            break;
          
          default:
            break;
          }
          setupMode=false;
        }

        //if (checkForPrintTimer.isTimerReady()==true){
        theAnswer=getFingerprintID();
        switch (theAnswer)
        {
        case FINGERPRINT_OK:
          digitalWrite(GREENLEDPIN,1);
          digitalWrite(REDLEDPIN,0);
          userName = "";
          Serial.printf("Sending user web request U=%i E=%i",userID,equipID);
          sendLoginRequest(userID,equipID);
          theState = 2;
          break;
        
        case FINGERPRINT_NOTFOUND:
          digitalWrite(GREENLEDPIN,0);
          digitalWrite(REDLEDPIN,1);
          theState = 3;rejectTimer.startTimer(5000);
          break;

        default:
          //don't do anything
          break;
        }  
          //checkForPrintTimer.startTimer(1000);
        //}
        break;

      case 2: //verifying login
        if (userName != ""){
          if(userName == "Unauthorized"){
            userName = "";userID = -1;
            theState = 3;
            rejectTimer.startTimer(5000);
          }
          else{
            //you've got a valid login
            startTime = Time.timeStr();
            cumCurrent = 0;theState = 4;

            //send initial record, and then start time for next writes
            sendData(userID,equipID,0,0);
            sendDataTimer.startTimer(30000);
          }
        }
        break;
      
      case 3: //rejecting login
        if(rejectTimer.isTimerReady()==true){theState = 1;}
        break;

      case 4: //Logged in is green
        if(sendDataTimer.isTimerReady()==true){

          //only send data if it's changed
          if(lastCurrent != cumCurrent){
            Serial.printf("Usage user %i, equip %i, curr %i, type %i\n",userID,equipID,cumCurrent,5);
            sendData(userID,equipID,cumCurrent,5);
            lastCurrent = cumCurrent;
          }
          else{
            Serial.printf("Supress usage record, no change from %i\n",cumCurrent);
          }
          sendDataTimer.startTimer(30000);
        }

        break;

      case 5: //logging out is blinking yellow
        break;

      default:
        break;
    }

}

void sendLoginRequest(int sFingerID,int sEquipID){
  //this sends a login request to the web service
  Particle.publish(EVENT_NAME, String::format("{\"finger\":%i,\"equip\":%i}", sFingerID, sEquipID), PRIVATE);
}

void sendData(int sFingerID,int sEquipID,int sCurrent,int sType){
  //this sends a login request to the web service
  Particle.publish(EVENT_NAME_ACTIVE, String::format("{\"finger\":%i,\"equip\":%i,\"current\":%i,\"type\":%i}", 
    sFingerID, sEquipID,sCurrent,sType), PRIVATE);
}

void subscriptionHandler_ACTIVE(const char *event_active, const char *data_active) {
  //This deteremines if data was written successfully
  if (data_active==userName){Serial.printf("Usage record success %s\n",data_active);}
}

void subscriptionHandler(const char *event, const char *data) {
  //This gets launched when the login request returns a value
  userName = data;
  Serial.printf("data = %s\n",data);
}

void setStatusPixel()
{
  if(pixelTimer.isTimerReady()==true){
    switch (theState){
      
      case 0: //startup is blinking red
          pixelOnOff = !pixelOnOff;
          if(pixelOnOff==true){
            pixel.setPixelColor(0,red);
            pixel.show();
          }
          else{
            pixel.setPixelColor(0,black);
            pixel.show();
          }
        break;
      
      case 1: //waiting for login is blue
        pixel.setPixelColor(0,blue);
        pixel.show();
        break;

      case 2: //verifying login is blinking blue
        pixelOnOff = !pixelOnOff;
        if(pixelOnOff==true){
          pixel.setPixelColor(0,blue);
          pixel.show();
        }
        else{
          pixel.setPixelColor(0,black);
          pixel.show();
        }
        break;
      
      case 3: //rejecting login blinking red
        pixelOnOff = !pixelOnOff;
        if(pixelOnOff==true){
          pixel.setPixelColor(0,red);
          pixel.show();
        }
        else{
          pixel.setPixelColor(0,black);
          pixel.show();
        }
        break;

      case 4: //Logged in is green
        pixel.setPixelColor(0,green);
        pixel.show();
        break;

      case 5: //logging out is blinking yellow
        pixelOnOff = !pixelOnOff;
        if(pixelOnOff==true){
          pixel.setPixelColor(0,yellow);
          pixel.show();
        }
        else{
          pixel.setPixelColor(0,black);
          pixel.show();
        }
        break;

      default:
        break;
    }
    pixelTimer.startTimer(500);
  }
}

void displayText()
{
  if(timerDisplay.isTimerReady()==true || bolDisplay==true){
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);

    switch (theState)
    {
    case 0: //startup
      display.setCursor(0,0);
      display.printf("Starting");
      break;
    
    case 1: //waiting for login
      display.setCursor(10,0);
      display.printf("Please");
      display.setCursor(10,20);
      display.printf("Login");
      break;

    case 2: //verifying login
      
      if(userName==""){
        display.setCursor(10,0);
        display.printf("Verifying");
        display.setCursor(10,20);
        display.printf("Login");
      }
      break;

    case 3: //rejecting login
      display.setCursor(10,0);
      display.printf("User not");
      display.setCursor(0,20);
      display.printf("Authorized");
      break;

    case 4: //Logged in

      TimeOnly = startTime.substring (11,19) ;
      display.setTextSize(1);
      display.setCursor(0,0);
      display.printf("User:%s", userName.c_str());
      display.setCursor (0,30);
      display.printf("Start: %s", TimeOnly.c_str());
      display.setCursor (0,50);
      display.printf("Cum Use:%i",cumCurrent);
      break;

    case 5: //logging out
      display.setCursor(0,0);
      display.setTextSize(2);
      display.printf("Log out");
      display.setCursor (0,30);
      display.setTextSize(1);
      display.printf("User:%s", userName.c_str());
      display.setCursor (0,40);
      display.printf("Cum Use:%i",cumCurrent );
      break;

    default:
      break;
    }
    display.display();

  }
  timerDisplay.startTimer(secDisplay);
  bolDisplay=false;
}

uint8_t getFingerID() {
  
  //return values
  //   2000  if nothing happened
  //   1000  if the fingerprint wasn't matched
  //   {id} if the fingerprint was matched

  int retVal = 2000;
  uint8_t p = finger.getImage();

  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    default:
      Serial.println("Finger image error");
      return retVal;
  }

  // OK success!
  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    default:
      Serial.println("Finger conversion error");
      return retVal;
  }
  
  // OK converted!
  retVal = 1000;
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } 
  else {
    Serial.println("Unknown match error");
    return retVal;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  return finger.fingerID;
}

uint8_t getFingerprintID() {
  
  uint8_t p = finger.getImage();
  //Serial.printf("getFingerprintID %i\n",p);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected\n");
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
  userID=int(finger.fingerID);
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 
  return p;
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

int getNextFree()
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


