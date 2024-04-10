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

// direct api call


// web hook data
  const char *EVENT_NAME = "GetUsername";
  int pFingerID=1, pEquipID=2;
  void subscriptionHandler(const char *event, const char *data);
  String userSubscription= "hook-response/GetUsername";

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
  Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
  IoTTimer oneSecTimer;
  bool setupMode = false;
  Button toggleButton(FINGERMODEPIN,false);
  uint8_t getFingerprintID();
  uint8_t getFingerprintEnroll(uint8_t id);
  uint8_t fingerID;
  uint8_t nextFree();
  int getFingerprintIDez();
  Button logOff (D12,true);
  boolean loggedOn = false;

//Program flow variables
  const int TIMEZONE = -6;
  const int BUTTONLOGOFF=D12;
  int theState = 0;
  String userName;
  int cumCurrent = 0;
  String startTime="", finishTime="";
  Button logOffButton(BUTTONLOGOFF,true);

//test sub routines
  void testRelay();
  uint8_t testFingerPrint();
  void testCurrentReader();

//all the subroutines
  void programLogic();
  void setStatusPixel();
  void displayText();
  String testRequest();
  void testHook();

  SYSTEM_MODE(AUTOMATIC);

void setup() {

  //get neopixel ready to display
    pixel.begin();
    pixel.setBrightness(22);
    pixelTimer.startTimer(0);

  //start the oled display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    display.clearDisplay();

  //start the serial monitor
    Serial.begin(9600);
    waitFor (Serial.isConnected,10000);

  //get current time
    Time.zone (-6);
    Particle.syncTime();

  // send tcp request
    String theName = testRequest();

  //web hook
    Particle.subscribe("hook-response/GetUsername", subscriptionHandler, MY_DEVICES);
    testHook();
  
}


void loop() { 

  //set the oled text based on the state
  //displayText();

  //set the status neo pixel based on the state
  //setStatusPixel();

  //react to the user pushing the logoff button (only if they are logged in)
  //if(logOffButton.isClicked()==true && theState==3){theState = 4;}

  //this is the main program logic  
  //programLogic();

}

void testHook(){
  //Particle.publish(EVENT_NAME, "", PRIVATE);
  Particle.publish(EVENT_NAME, String::format("{\"finger\":%i,\"equip\":%i}", 7, 2), PRIVATE);
}

void subscriptionHandler(const char *event, const char *data) {
    
    Serial.printf("data = %s\n",data);
}

String testRequest()
{
  byte server[] = {20,119,0,43}; 
  TCPClient theTCPClient;
  uint16_t thePort = 443;
  String theHost = "testserviceapi2.azure-api.net";
  String theCall = "/UserAuthorized/UserAuthorized";
  String theParameters = "?FingerID=1&EquipID=2";
  String retString;

  if (theTCPClient.connect(server, thePort)){
    Serial.printf("Connected to %s\n",theHost.c_str());

      theTCPClient.println("GET https://testserviceapi2.azure-api.net/UserAuthorized/UserAuthorized?FingerID=1&EquipID=2 HTTP/1.1 ");
      theTCPClient.println("Host: testserviceapi2.azure-api.net ");
      theTCPClient.println("accept: text/plain");
      
      retString = theTCPClient.readString();
      Serial.printf("Returned %s\n",retString.c_str());

      // theTCPClient.print(theCall);
      // theTCPClient.print(theParameters);
      // Serial.printf("Before Read\n");
      // retString = theTCPClient.readString();
      // Serial.printf("Returned %s\n",retString.c_str());
    
  }
  
  //theTCPClient.connect("https://testserviceapi2.azure-api.net/UserAuthorized/UserAuthorized?FingerID=1&EquipID=2",thePort); 
  

  // if (theTCPClient.connect(theWebCall, thePort)) {
  //   theTCPClient.print("/UserAuthorized/UserAuthorized");
  //   theTCPClient.print("?FingerID=1&EquipID=2");
  //   retString = theTCPClient.readString();
  // }
  return retString;
}

void programLogic()
{
  switch (theState){
      
      case 0: //startup is blinking red

        break;
      
      case 1: //waiting for login is blue

        break;

      case 2: //verifying login is blinking blue
        break;
      
      case 3: //Logged in is green
        break;

      case 4: //logging out is blinking yellow
        break;

      default:
        break;
    }

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
      
      case 3: //Logged in is green
        pixel.setPixelColor(0,green);
        pixel.show();
        break;

      case 4: //logging out is blinking yellow
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
      display.printf("Starting up");
      break;
    
    case 1: //waiting for login
      display.setCursor(0,0);
      display.printf("Please login");
      break;

    case 2: //verifying login
      display.setCursor(0,0);
      display.printf("Verifying login");
      break;
    
    case 3: //Logged in
      display.setCursor(0,0);
      display.printf("User:   %s", userName.c_str());
      display.setCursor (0,20);
      display.printf("Start: %s", startTime.c_str());
      display.setCursor (0,30);
      display.printf("Cum Use:%i",cumCurrent );
      break;

    case 4: //logging out
      display.setCursor(0,0);
      display.printf("Logging out of device");
      display.setCursor (0,20);
      display.printf("User:   %s", userName.c_str());
      display.setCursor (0,30);
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
