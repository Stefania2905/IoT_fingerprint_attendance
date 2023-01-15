#include <SPI.h>
#include <WiFiNINA.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include <SoftwareSerial.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define ENROLL_TIME 12000
#define PRINTS_MAX 200
#include <GT_521F.h>

SoftwareSerial mySerial (0, 1);
SoftwareSerial mySerial1(7, 8);

GT_521F fps(mySerial); 

rgb_lcd lcd;
WiFiClient client;

const int buttonPin = 2;
const int buttonPin_second = 5;

int buttonState = 0;


const char ssid[] = "BT-GNAF3G";
const char password[] = "8DortsCrescent";
const unsigned long channel_id = 1959270;
const char write_api_key[] = "ZU4YUNGRKH3H0ATY";


const int colorR = 255;
const int colorG = 0;
const int colorB = 0;


 unsigned long last_sensor_trip[]={0,0};

void setup() 
{
    Serial.begin(9600);
    mySerial1.begin(9600);
    fps.begin(9600);   
    Serial.print("Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);
    Serial.println();
    Serial.println("Connected!");
    ThingSpeak.begin(client);

   
    lcd.begin(16, 2);
    setDefaultLCDtext();
  
    Serial.println("PUT FINGER ON SENSOR");
    
    pinMode(buttonPin, INPUT);
    pinMode(buttonPin_second, INPUT);


    timeClient.begin();
    timeClient.setTimeOffset(0);
    timeClient.update();

  last_sensor_trip[0] = timeClient.getEpochTime();
  last_sensor_trip[1] = timeClient.getEpochTime();
 
}

void setDefaultLCDtext()
{
    lcd.clear();
    lcd.setRGB(0, 255, 0);
    lcd.setCursor(0,0);
    lcd.print("Hi,please scan");
    lcd.setCursor(0,1);
    lcd.print("your finger");

}

void button_enrollment(){

buttonState = digitalRead(buttonPin);
 
    
if (buttonState == HIGH) {
       
 uint16_t State = FingerPrintEnrollment();
        if(State==NO_ERROR)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Added ID: ");
          lcd.print(fps.getEnrollCount()-1);

            
        }
        else
        {
          Serial.print("Add Finger Fail ERROR: ");
          Serial.println(State);
        }


  }

}

void checkPush()
{
 int pushed = digitalRead(buttonPin_second);  // read input value

  if (pushed == HIGH) {
    menu_1();
    while (Serial.available() > 0) {
      char check = Serial.read();
     
      if (check == '0') {
        menu_1();
      } else if (check == '1') {
        fps.open(false);
        uint16_t fingerPrintCount = fps.getEnrollCount();
        if (fingerPrintCount < NACK_TIMEOUT) {
          Serial.print("EnrollCount: ");
          Serial.println(fingerPrintCount);
        } else {
          Serial.print("EnrollCount FAIL: ");
          Serial.println(fingerPrintCount);
        }
      } 
     
     else if (check == '2') {
        uint16_t openStatus = fps.open(true);
        if (openStatus == NO_ERROR) {
          uint16_t checkLED = fps.cmosLed(true);
          if (checkLED == NO_ERROR) {
            Serial.println("Place finger you want to check on Sensor");
            uint32_t FingerCountTime = millis();
            uint16_t checkFinger = FINGER_IS_NOT_PRESSED;
            while ((checkFinger == FINGER_IS_NOT_PRESSED) && ((millis() - FingerCountTime) < ENROLL_TIME)) {
              delay(100);
              checkFinger = fps.isPressFinger();
            }
            if (checkFinger == FINGER_IS_PRESSED) {
              Serial.println("-FINGER IS PRESSED");
              checkFinger = fps.captureFinger();
              if (checkFinger == NO_ERROR) {
                Serial.println("-FINGER CAPTURED");
                checkFinger = fps.identify();
                if (checkFinger < PRINTS_MAX) {
                  fps.cmosLed(false);
                  Serial.print("-FINGER FOUND ID: ");
                  Serial.println(checkFinger);
                } else {
                  Serial.println("-FINGER NOT FOUND");
                }
              } else {
                Serial.println("-FINGER CAPTURE FAILED");
              }
            } else {
              Serial.print("-FINGER FAIL: ");
              Serial.println(checkFinger, HEX);
            }
          } else {
            Serial.print("-LED FAIL: ");
            Serial.println(checkLED, HEX);
          }
        } else {
          Serial.print("-Initialization failed!\nstatus: ");
          Serial.print(openStatus, HEX);
          Serial.println();
        }
      } else if (check == '3') {
        uint16_t State = fps.deleteAll();
        if (State == NO_ERROR) {
          Serial.println("-All Prints Deleted");
        } else {
          Serial.print("-Delete Failed: ");
          Serial.println(State, HEX);
        }
      } 
    }
  }

}

void updateSerial()
{
  delay(500);
  while (Serial.available()) 
  {
    mySerial1.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(mySerial1.available()) 
  {
    Serial.write(mySerial1.read());//Forward what Software Serial received to Serial Port
  }
}

void loop() 
{
  timeClient.update();
  updateSerial();  

 auto epochTime = timeClient.getEpochTime();

String formattedDate = timeClient.getFormattedTime();

int currentHour = timeClient.getHours();
int currentMinute = timeClient.getMinutes();
int currentSecond = timeClient.getSeconds();
   
uint16_t openStatus = fps.open(true);

  if(openStatus == NO_ERROR)
    {
    
      uint16_t checkLED = fps.cmosLed(true);
      if(checkLED == NO_ERROR)
      {
          
        uint16_t checkFinger = FINGER_IS_NOT_PRESSED;
        while (true)
        {
          checkPush();
          button_enrollment(); 
       
          checkFinger = fps.isPressFinger();
          if(checkFinger == FINGER_IS_PRESSED)
          {
      
            checkFinger = fps.captureFinger();
            if(checkFinger == NO_ERROR)
            {
              checkFinger = fps.identify();
              if(checkFinger < 200)
              {
                 lcd.clear();
                 lcd.setCursor(0, 0);
                lcd.print("FINGER ID: ");
                lcd.print(checkFinger);
                delay(2000);

                lcd.clear();
                int test = checkFinger;
                lcd.print("Hello,");
                lcd.setCursor(0,1);
                lcd.print(test);
                Serial.println(test);



ThingSpeak.setField(1, formattedDate);

ThingSpeak.setField(2, test);

ThingSpeak.writeFields(channel_id, write_api_key);

delay(3000);
              }

            }
       
          }
      
        }
      }
      else
      {
        Serial.print("LED FAIL: ");
        Serial.println(checkLED,HEX);
      }
    }
   
    fps.cmosLed(false);
 
}

void menu_1()
{
   Serial.println("Menu Options:");
  Serial.println("1: Check FingerPrint Count");
  //Serial.println("2: Add FingerPrint");
  Serial.println("2: Check if Finger is enrolled");
  Serial.println("3: Remove All FingerPrints");
  //Serial.println("Option Invalid");
}



uint8_t FingerPrintEnrollment()
{
  uint8_t enrollState = NO_ERROR;
  uint16_t enrollid = 0;
  uint32_t enrollTimeOut = 0;
  enrollState = fps.open(false);
  if(enrollState == NO_ERROR)
  {
    Serial.println("Starting Enrollment");
    enrollState = ID_IS_ENROLLED;
    while (enrollState == ID_IS_ENROLLED)
    {
      enrollState = fps.checkEnrolled(enrollid);
      if (enrollState==ID_IS_ENROLLED) 
      {
        enrollid++;
      }
      else if(enrollState != ID_IS_NOT_ENROLLED)
      {
        Serial.print("ID Error: "); 
        Serial.print(enrollid); 
        Serial.print(" : ");
        Serial.println(enrollState,HEX); 
      }
      delay(1);
    }
    if(enrollState == ID_IS_NOT_ENROLLED)
    {

        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("ID Cleared: "); 
        lcd.print(enrollid);

      enrollState = fps.enrollStart(enrollid);
      if(enrollState == NO_ERROR)
      {
        enrollState = fps.cmosLed(true);
        if(enrollState==NO_ERROR)
        {
          for(int i = 1;i<4;i++)
          {
            enrollState = fps.cmosLed(true);
            enrollTimeOut = millis();
            Serial.print(i);
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Press Finger");
          
            enrollState = FINGER_IS_NOT_PRESSED;
            while((enrollState != FINGER_IS_PRESSED) && ((millis()-enrollTimeOut)<ENROLL_TIME))
            {
              delay(100);
              enrollState = fps.isPressFinger();
            }                    
            if(enrollState == FINGER_IS_PRESSED)
            {

           lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FINGER IS PRESSED");

              enrollState = NO_ERROR;
              enrollState = fps.captureFinger(1);
              if(enrollState==NO_ERROR)
              {
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FINGER Captured");

                enrollState = fps.enrollFinger(i);
                if(enrollState == NO_ERROR)
                {  
                   lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Finger Enrolled");

                  
                }
                else
                {
                  Serial.print("Enroll Failed: ");
                  Serial.println(enrollState,HEX);
                }                
              }
              else
              {
                Serial.print("Capture Failed: ");
                Serial.println(enrollState,HEX);
                break;
              }
              enrollTimeOut = millis();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Remove Finger");

              enrollState = FINGER_IS_PRESSED;
              while((enrollState == FINGER_IS_PRESSED) && ((millis()-enrollTimeOut)<ENROLL_TIME))
              {
                enrollState = fps.isPressFinger();
                delay(5);
              } 
             // fps.cmosLed(false);
              if((millis()-enrollTimeOut)>ENROLL_TIME)
              {
                Serial.println("Did not Remove Finger: TimeOut");
                break;
              }
              enrollState = NO_ERROR;              
            }
            else
            {
              Serial.println("Finger pressed TimeOut");
              break;
            }
            //fps.cmosLed(false);
            delay(1000);
          } //End of For loop
          
          if(enrollState == NO_ERROR)
          {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("DONE");
      
          }
        }
      }
      else
      {
        Serial.print("Enrolling Start Failed: "); 
        Serial.println(enrollState,HEX);
      }
    }
    else
    {
      Serial.print("Enrolling ID Fail: "); 
      Serial.println(enrollState,HEX); 
    }
  }
  else
  {
    Serial.print("Enrolling Fail: "); 
    Serial.println(enrollState,HEX); 
  }
  //fps.cmosLed(false);
  return enrollState;
}


void printLCD(String message)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}
