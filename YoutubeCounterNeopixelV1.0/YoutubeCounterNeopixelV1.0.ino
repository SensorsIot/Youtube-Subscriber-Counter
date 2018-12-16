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
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <YoutubeApi.h>
#include <MusicEngine.h>
#include <SNTPtime.h>
#include <IOTAppStory.h>
#include <credentials.h>

#define COMPDATE __DATE__ __TIME__
#define MODEBUTTON D3
// IotAppStory.com library
IOTAppStory IAS(COMPDATE, MODEBUTTON);  // Initialize IotAppStory


// ================================================ PIN DEFINITIONS ======================================


#define BUZ_PIN D5
#define NEO_PIN D4
#define POWER_PIN D1

#define NUMBER_OF_DIGITS 6
#define STARWARS "t112v127l12<dddg2>d2c<ba>g2d4c<ba>g2d4cc-c<a2d6dg2>d2c<ba>g2d4c<ba>g2d4cc-c<a2"

#define SUBSCRIBER_INTERVAL 30000
#define NTP_LOOP_INTERVAL 60000
#define DISP_LOOP_INTERVAL 500

#define MAX_BRIGHTNESS 80

strDateTime timeNow;
int lastHour = 0;

unsigned long entrySubscriberLoop, entryNTPLoop, entryDispLoop;

struct subscriberStruc {
  long last;
  long actual;
  long old[24];
} subscribers;

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 6, 1, NEO_PIN,
                            NEO_TILE_TOP   + NEO_TILE_LEFT   + NEO_TILE_ROWS   + NEO_TILE_PROGRESSIVE +
                            NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255), matrix.Color(255, 0 , 255)
};

MusicEngine music(BUZ_PIN);

WiFiClientSecure client;
YoutubeApi api(API_KEY, client);
SNTPtime NTPch("ch.pool.ntp.org");



// ================================================ SETUP ================================================
void setup() {
  pinMode(POWER_PIN, INPUT);
  Serial.println("Start");

  matrix.begin();
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);
  matrix.show();
  displayText("YouTube");

  IAS.preSetDeviceName("YouTubeNeo");
  IAS.preSetAutoUpdate(true);                      // automaticUpdate (true, false)
  IAS.preSetAutoConfig(false);                      // automaticConfig (true, false)
  IAS.preSetWifi(mySSID, myPASSWORD);

  IAS.onModeButtonShortPress([]() {
    Serial.println(F(" If mode button is released, I will enter in firmware update mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onModeButtonLongPress([]() {
    Serial.println(F(" If mode button is released, I will enter in configuration mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onFirmwareUpdateCheck([]() {
    Serial.println(F(" Checking if there is a firmware update available........."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
    displayText("Checking");

  });

  IAS.onFirmwareUpdateDownload([]() {
    Serial.println(F(" Downloading and Installing firmware update."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
    displayText("Update");
  });

  IAS.onFirmwareUpdateError([]() {
    Serial.println(F(" Update failed...Check your logs"));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onConfigMode([]() {
    Serial.println(F(" Starting configuration mode. Search for my WiFi and connect to 192.168.4.1."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
    displayText("Config");
  });


  IAS.onFirmwareUpdateCheck([]() {
    Serial.println(F(" Checking if there is a firmware update available."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
    displayText("Checking Update");
  });

  IAS.begin('P');
  IAS.setCallHome(false);                // Set to true to enable calling home frequently (disabled by default)
  IAS.setCallHomeInterval(86400);   // Call home interval in seconds

  pinMode(A0, INPUT);

  while (!NTPch.setSNTPtime()) Serial.print("."); // set internal clock
  entryNTPLoop = millis() + NTP_LOOP_INTERVAL;
  entrySubscriberLoop = millis() + SUBSCRIBER_INTERVAL;
  beepUp();
  beepUp();
  while (subscribers.actual == 0) {
    api.getChannelStatistics(CHANNEL_ID);
    subscribers.actual = api.channelStats.subscriberCount;
  }
  for (int i = 0; i < 24; i++) {
    if (subscribers.old[i] == 0) subscribers.old[i] = subscribers.actual - 245;
  }
  debugPrintSubs();
  Serial.println("Setup done");
}


// ================================================ LOOP =================================================
void loop() {
  IAS.loop();                                            // this routine handles the reaction of the Flash button. If short press: update of skethc, long press: Configuration

  //-------- Your Sketch starts from here ---------------

  if (millis() - entryDispLoop > DISP_LOOP_INTERVAL) {
    entryDispLoop = millis();
    displayNeo(subscribers.actual, subscribers.actual - subscribers.old[timeNow.hour]);
  }

  if (millis() - entryNTPLoop > NTP_LOOP_INTERVAL) {
    //   Serial.println("NTP Loop");
    entryNTPLoop = millis();
    timeNow = NTPch.getTime(1.0, 1); // get time from internal clock
    NTPch.printDateTime(timeNow);
    if (timeNow.hour != lastHour ) {
      Serial.println("New hour!!!!!!!");
      subscribers.old[lastHour] = subscribers.actual;
      subscribers.last = subscribers.actual;
      debugPrintSubs();
      lastHour = timeNow.hour;
    }
  }

  if (millis() - entrySubscriberLoop > SUBSCRIBER_INTERVAL) {
    //   Serial.println("Subscriber Loop");
    entrySubscriberLoop = millis();
    updateSubs();
  }
}

void updateSubs() {
  if (api.getChannelStatistics(CHANNEL_ID))
  {
    // get subscribers from YouTube
    //      Serial.println("Get Subs");
    subscribers.actual = api.channelStats.subscriberCount;
    displayNeo(subscribers.actual, subscribers.actual - subscribers.old[timeNow.hour]);
    Serial.print("Subs ");
    Serial.print(subscribers.actual);
    Serial.print(" yesterday ");
    Serial.println(subscribers.old[timeNow.hour]);
    if (subscribers.last > 0) {
      if (subscribers.actual > subscribers.last ) {
        beepUp();
        if (subscribers.actual % 10 <= subscribers.last % 10) for (int ii = 0; ii < 1; ii++) beepUp();
        if (subscribers.actual % 100 <= subscribers.last % 100) starwars();
        if (subscribers.actual % 1000 <= subscribers.last % 1000) for (int ii = 0; ii < 2; ii++) starwars();
      }
      else {
        if (subscribers.actual < subscribers.last ) beepDown();
      }
    }
    subscribers.last = subscribers.actual;
    //     debugPrint();
  }
}


void displayNeo(int subs, int variance ) {
  // Serial.println("Display");

  matrix.fillScreen(0);
  int bright = measureLight();
  if (bright > -1) {
    matrix.setBrightness(bright);
    matrix.setTextColor(colors[2]);
    matrix.setCursor(0, 0);
    matrix.print(String(subs));

    // Show arrow
    char arrow = (variance <= 0) ? 0x1F : 0x1E;
    if (variance > 0) matrix.setTextColor(colors[1]);
    else matrix.setTextColor(colors[0]);
    matrix.setCursor(36, 0);
    matrix.print(arrow);

    // show variance bar
    int h = map(variance, 0, 400, 0, 8);
    h = (h > 8) ? 8 : h;
    if (h > 0) matrix.fillRect(42, 8 - h,  1, h , colors[3]);
  }
  matrix.show();
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

int measureLight() {
  int brightness = map(analogRead(A0), 360, 800, MAX_BRIGHTNESS, 0);
  brightness = (brightness > MAX_BRIGHTNESS) ? MAX_BRIGHTNESS : brightness;  // clip value
  brightness = (brightness < 20) ? 0 : brightness;
 // brightness = MAX_BRIGHTNESS;
  return brightness;
}

void debugPrint() {
  Serial.println("---------Stats---------");
  Serial.print("Subscriber Count: ");
  Serial.println(subscribers.actual);
  Serial.print("Variance: ");
  Serial.println(subscribers.actual - subscribers.old[timeNow.hour]);
  Serial.print("LastSubs: ");
  Serial.println(subscribers.last);
  Serial.print("View Count: ");
  Serial.println(api.channelStats.viewCount);
  Serial.print("Comment Count: ");
  Serial.println(api.channelStats.commentCount);
  Serial.print("Video Count: ");
  Serial.println(api.channelStats.videoCount);
  // Probably not needed :)
  Serial.print("hiddenSubscriberCount: ");
  Serial.println(subscribers.actual);
  Serial.println("------------------------");
}
void debugPrintSubs() {
  for (int i = 0; i < 24; i++) {
    Serial.print(i);
    Serial.print(" old ");
    Serial.println(subscribers.old[i]);
  }
}

void displayText(String tt) {
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.fillScreen(0);
  matrix.setTextColor(colors[0]);
  matrix.setCursor(0, 0);
  matrix.print(tt);
  matrix.show();
}

