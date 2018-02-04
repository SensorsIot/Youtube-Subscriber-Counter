/* This is an initial sketch to be used as a "blueprint" to create apps which can be used with IOTappstory.com infrastructure
  Your code can be filled wherever it is marked.


  Copyright (c) [2016] [Andreas Spiess]

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/
#define SERIALDEBUG         // Serial is used to present debugging messages
//#define DISPLAY

#include <YoutubeApi.h>
#include <SPI.h>
#include <MAX7219_Dot_Matrix.h>
#include <MusicEngine.h>
#include <SNTPtime.h>
#include <credentials.h>

#define SKETCH "YoutubeCounter "
#define VERSION "V1.6"
#define COMPDATE __DATE__ __TIME__

// ================================================ PIN DEFINITIONS ======================================
#ifdef ARDUINO_ESP8266_ESP01  // Generic ESP's 
#define MODEBUTTON 0
#define BUZ_PIN 5
#define CS_PIN 2
#define LEDgreen 13
//#define LEDred 12
#else
#define MODEBUTTON D3
#define BUZ_PIN D1
#define CS_PIN D4
#define LEDgreen D7
//#define LEDred D6
#endif


#define NUMBER_OF_DIGITS 5
#define STARWARS "t112v127l12<dddg2>d2c<ba>g2d4c<ba>g2d4cc-c<a2d6dg2>d2c<ba>g2d4c<ba>g2d4cc-c<a2"



int rawIntensity, intensity;
long lastSubscribers, subscribers;
unsigned long entrySubscriberLoop;
strDateTime dateTime;
int nextRound = 0;

#include <IOTAppStory.h>
IOTAppStory IAS(SKETCH, VERSION, COMPDATE, MODEBUTTON);

MAX7219_Dot_Matrix myDisplay (NUMBER_OF_DIGITS, CS_PIN); // # digits, and then specify the CS pin

MusicEngine music(BUZ_PIN);

WiFiClientSecure client;
YoutubeApi api(API_KEY, client);
SNTPtime NTPch("ch.pool.ntp.org");

// ================================================ SETUP ================================================
void setup() {
  IAS.serialdebug(true);
  Serial.println("Start");
  myDisplay.begin();
  myDisplay.setIntensity (15);

  IAS.preSetBoardname("YouTube");
  IAS.preSetAutoUpdate(false);                      // automaticUpdate (true, false)
  IAS.preSetAutoConfig(true);                      // automaticConfig (true, false)
  IAS.preSetWifi(mySSID, myPASSWORD);

  IAS.onModeButtonShortPress([]() {
    Serial.println(" If mode button is released, I will enter in firmware update mode.");
    Serial.println("*------------------------------------------------------------------------ -*");
    dispMatrix("UPD");
  });

  IAS.onModeButtonLongPress([]() {
    Serial.println(" If mode button is released, I will enter in configuration mode.");
    Serial.println("*------------------------------------------------------------------------ -*");
    dispMatrix("UPD");
  });

  IAS.onModeButtonVeryLongPress([]() {
    Serial.println(" If mode button is released, I won't do anything.");
    Serial.println("*-------------------------------------------------------------------------*");
    dispMatrix("UPD");
  });

  IAS.begin(true, 'P');

  dispMatrix(VERSION);
  pinMode(A0, INPUT);

  while (!NTPch.setSNTPtime()) Serial.print("."); // set internal clock
  Serial.println("Setup done");
  delay(3000);
}


// ================================================ LOOP =================================================
void loop() {
  yield();
  IAS.buttonLoop();                                            // this routine handles the reaction of the Flash button. If short press: update of skethc, long press: Configuration


  //-------- Your Sketch starts from here ---------------

  myDisplay.setIntensity (adjustIntensity());

  if (millis() - entrySubscriberLoop > nextRound) {
    entrySubscriberLoop = millis();

    if (api.getChannelStatistics(CHANNEL_ID))
    {
      // get subscribers from YouTube
      subscribers = api.channelStats.subscriberCount;
      dispString(String(subscribers));

      if ((subscribers > lastSubscribers) && (lastSubscribers > 0) ) {
        beepUp();
        if (subscribers % 10 <= lastSubscribers % 10) for (int ii = 0; ii < 1; ii++) beepUp();
        if (subscribers % 100 <= lastSubscribers % 100) starwars();
        if (subscribers % 1000 <= lastSubscribers % 1000) for (int ii = 0; ii < 3; ii++) starwars();
      } else if (subscribers < lastSubscribers) beepDown();
      lastSubscribers = subscribers;
      Serial.println("---------Stats---------");
      Serial.print("Subscriber Count: ");
      Serial.println(api.channelStats.subscriberCount);
      Serial.print("View Count: ");
      Serial.println(api.channelStats.viewCount);
      Serial.print("Comment Count: ");
      Serial.println(api.channelStats.commentCount);
      Serial.print("Video Count: ");
      Serial.println(api.channelStats.videoCount);
      // Probably not needed :)
      Serial.print("hiddenSubscriberCount: ");
      Serial.println(subscribers);

      Serial.println("------------------------");
      nextRound = 10000;
    }
  }
}


void dispMatrix(String content) {
#ifdef DISPLAY
  Serial.println("dispMatrix");
  char charBuf[50];
  content.toCharArray(charBuf, 50);
  myDisplay.sendString(charBuf);
#endif
}


void beepUp() {
  music.play("T80 L40 O4 CDEFGHAB>CDEFGHAB");
  while (music.getIsPlaying() == 1) yield();
  delay(500);
}

void beepDown() {
  music.play("T1000 L40 O5 BAGFEDC<BAGFEDC<BAGFEDC");
  while (music.getIsPlaying() == 1) yield();
  delay(500);
}

void starwars() {
  music.play(STARWARS);
  while (music.getIsPlaying()) yield();
  delay(500);
}

int adjustIntensity() {
  rawIntensity = analogRead(A0);
  float _intensity = 15 - (15.0 / (560.0 - 0) * rawIntensity);
  int intensity = (_intensity > 15.0) ? 15 : (int)_intensity;
  intensity = (_intensity < 2.0) ? 0 : (int)_intensity;
  return intensity;
}

void dispString(String dispContent) {
  int intensity = adjustIntensity();

  // display
  if (dispContent.length() < 5) dispContent =  " " + dispContent;
  if (intensity == 0) dispContent = "     ";
  else myDisplay.setIntensity (intensity);
  dispMatrix(dispContent);
}

void dispTime() {
  dateTime = NTPch.getTime(1.0, 1); // get time from internal clock
  NTPch.printDateTime(dateTime);

  byte actualHour = dateTime.hour;
  byte actualMinute = dateTime.minute;
  String strMin = String(actualMinute);
  if (strMin.length() < 2) strMin = "0" + strMin;
  dispString(String(actualHour) + ":" + strMin);
}

