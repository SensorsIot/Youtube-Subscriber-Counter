#include <credentials.h>
#define DEBUG 1
#include <DebugUtils.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <SPI.h>
#include <MAX7219_Dot_Matrix.h>

#define FIRMWARE "SubscriberCounterV3"
#define FIRMWARE_VERSION FIRMWARE"_000"


extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}
#define BUZZER 5

#define NUMBER_OF_DEVICES 5
#define CS_PIN 2



typedef struct {
  byte markerFlag;
  long lastSubscribers;
  int updateSpaces;
  int runSpaces;
} rtcMemDef __attribute__((aligned(4)));
rtcMemDef rtcMem;

MAX7219_Dot_Matrix myDisplay (NUMBER_OF_DEVICES, CS_PIN); // 8 chips, and then specify the LOAD pin only

// declare telnet server
WiFiServer TelnetServer(23);
WiFiClient Telnet;

long subscribers, lastSubscribers;
WiFiClient client;

#include "ESP_Helpers.h"

int getSubscribers() {
  const char* host = "api.thingspeak.com";
  int subscribers = 0;

  // Establish connection to host
  Serial.printf("\n[Connecting to %s ... ", host);
  if (client.connect(host, 80))
  {
    Serial.println("connected]");

    // send GET request to host
    String url = "/apps/thinghttp/send_request?api_key=W7XYLLV4BOZHERBL";
    DEBUGPRINTLN1("[Sending a request]");
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    DEBUGPRINTLN1("[Response:]");
    while (client.connected())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        int hi = line.indexOf("subscribers");

        if (hi > 0) {
          String tt = line.substring(hi - 6, hi - 1);
          tt.replace(",", "");
          subscribers = tt.toInt();
          DEBUGPRINTLN1(subscribers);
        }
      }
    }
    client.stop();
    DEBUGPRINTLN1("\n[Disconnected]");
  }
  else
  {
    DEBUGPRINTLN1("connection failed!]");
    client.stop();
  }
  return subscribers;
}


void beep(int beepDuration) {
  digitalWrite(BUZZER, HIGH);
  delay(beepDuration);
  digitalWrite(BUZZER, LOW);
  delay(500);
}

void dispMatrix(String content) {
  char charBuf[50];
  content.toCharArray(charBuf, 50);
  myDisplay.sendString(charBuf);
}

void display(long num) {
  int hi = analogRead(A0);
  float _intensity = 15 - (15.0 / (560.0 - 0) * hi);
  int intensity = (_intensity > 15.0) ? 15 : (int)_intensity;
  intensity = (_intensity < 2.0) ? 0 : (int)_intensity;
  // DEBUGPRINT1("Intensity ");
  // DEBUGPRINTLN1(intensity);
  // for (int i=0;i<intensity;i++) beep(100);
  myDisplay.setIntensity (intensity);

  // display
  String dispContent = String(num);
  if (dispContent.length() < 5) dispContent =  " " + dispContent;
  DEBUGPRINT1(" dispContent ");
  DEBUGPRINTLN1(dispContent);
  dispMatrix(dispContent);

}

void setup() {
  int wiTry = 0;

  Serial.begin(115200);
  Serial.println("");
  DEBUGPRINTLN1("Start");
  pinMode(A0, INPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  readRTCmem();

  myDisplay.begin ();
  display(rtcMem.lastSubscribers);

  if (rtcMem.runSpaces >= RUNINTERVAL) {

    readCredentials();
    WiFi.mode(WIFI_STA);
    WiFi.begin(mySSID, myPASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      DEBUGPRINT1(".");
      if (wiTry >= 30) {
        ESP.reset();
      } else {
        wiTry++;
      }
    }

    if (iotUpdater(UPDATE_SERVER, UPDATE_URL, FIRMWARE_VERSION, false, true) == 0) {
      iotUpdater(SECOND_UPDATE_SERVER, SECOND_UPDATE_URL, FIRMWARE_VERSION, true, true);
    }

    TelnetServer.begin();
    TelnetServer.setNoDelay(true);

    subscribers = getSubscribers();
    if (subscribers < rtcMem.lastSubscribers) subscribers = rtcMem.lastSubscribers;
    // subscribers = rtcMem.lastSubscribers + 1;

    DEBUGPRINT1("-----");
    DEBUGPRINT1(rtcMem.lastSubscribers);
    DEBUGPRINT1(" ");
    DEBUGPRINTLN1(subscribers);
    if (subscribers > rtcMem.lastSubscribers) {
      beep(90);
      if (subscribers % 10 == 0) for (int ii = 0; ii < 1; ii++) beep(90);
      if (subscribers % 100 == 0) for (int ii = 0; ii < 3; ii++) beep(90);
      if (subscribers % 1000 == 0) for (int ii = 0; ii < 5; ii++) beep(90);
      if (subscribers == 10000) for (int ii = 0; ii < 100; ii++) beep(90);
    }
    display(subscribers);
    rtcMem.lastSubscribers = subscribers;
    rtcMem.runSpaces = 0;
  } else {
    rtcMem.runSpaces = ++rtcMem.runSpaces;
    writeRTCmem();
  }
  writeRTCmem();
  ESP.deepSleep(3000000);
}

void loop() {
  handleTelnet();

  int hi = analogRead(A0);
  float _intensity = 15 - (15.0 / (560.0 - 20) * hi);
  int intensity = (_intensity > 15.0) ? 15 : (int)_intensity;
  intensity = (_intensity < 1.0) ? 1 : (int)_intensity;
  DEBUGPRINT1("A0: " );
  DEBUGPRINT1(hi );
  DEBUGPRINT1(" Intensity  ");
  DEBUGPRINTLN1(intensity);

  Telnet.print("A0: " );
  Telnet.print(hi );
  Telnet.print(" Intensity  ");
  Telnet.println(intensity);

  delay(100);;
}
