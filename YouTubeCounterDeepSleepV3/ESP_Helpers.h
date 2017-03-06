


#define UPDATEINTERVAL 2
#define RUNINTERVAL 2

#define UPDATE_SERVER "192.168.0.200"
#define UPDATE_URL "/iotappstore/iotappstorev20.php"

#define SECOND_UPDATE_SERVER "iotappstore.org"
#define SECOND_UPDATE_URL "/iotappstore/iotappstorev20.php"

#define SSIDBASE 200
#define PASSWORDBASE 220

#define MAGICBYTE 85
#define RTCMEMBEGIN 68

char ssid[20];
char password[20];

String initSSID = mySSID;
String initPassword = myPASSWORD;

void handleTelnet() {
  if (TelnetServer.hasClient()) {
    // client is connected
    if (!Telnet || !Telnet.connected()) {
      if (Telnet) Telnet.stop();         // client disconnected
      Telnet = TelnetServer.available(); // ready for new client
    } else {
      TelnetServer.available().stop();  // have client, block new conections
    }
  }
}

void  printRTCmem() {
  DEBUGPRINTLN1("");
  DEBUGPRINTLN1("rtcMem ");
  DEBUGPRINT1("markerFlag ");
  DEBUGPRINTLN1(rtcMem.markerFlag);
  DEBUGPRINT1("runSpaces ");
  DEBUGPRINTLN1(rtcMem.runSpaces);
  DEBUGPRINT1("updateSpaces ");
  DEBUGPRINTLN1(rtcMem.updateSpaces);
  DEBUGPRINT1("lastSubscribers ");
  DEBUGPRINTLN1(rtcMem.lastSubscribers);
}


void readRTCmem() {
  system_rtc_mem_read(RTCMEMBEGIN, &rtcMem, sizeof(rtcMem));
  if (rtcMem.markerFlag != MAGICBYTE || rtcMem.lastSubscribers < 0 ) {
    rtcMem.markerFlag = MAGICBYTE;
    rtcMem.lastSubscribers = 0;
    rtcMem.updateSpaces = 0;
    rtcMem.runSpaces = 0;
    system_rtc_mem_write(RTCMEMBEGIN, &rtcMem, sizeof(rtcMem));
  }
  printRTCmem();
}

void writeRTCmem() {
  rtcMem.markerFlag = MAGICBYTE;
  system_rtc_mem_write(RTCMEMBEGIN, &rtcMem, sizeof(rtcMem));
}

bool readCredentials() {
  EEPROM.begin(512);
  if (EEPROM.read(SSIDBASE - 1) != 0x5)  {
    Serial.println(EEPROM.read(SSIDBASE - 1), HEX);
    initSSID.toCharArray(ssid, initSSID.length() + 1);
    for (int ii = 0; ii <= initSSID.length(); ii++) EEPROM.write(SSIDBASE + ii, ssid[ii]);

    initPassword.toCharArray(password, initPassword.length() + 1);
    for (int ii = 0; ii <= initPassword.length(); ii++) EEPROM.write(PASSWORDBASE + ii, password[ii]);
    EEPROM.write(SSIDBASE - 1, 0x35);
  }
  int i = 0;
  do {
    ssid[i] = EEPROM.read(SSIDBASE + i);
    i++;
  } while (ssid[i - 1] > 0 && i < 20);

  if (i == 20) DEBUGPRINTLN1("ssid loaded");
  i = 0;
  do {
    password[i] = EEPROM.read(PASSWORDBASE + i);
    i++;
  } while (password[i - 1] != 0 && i < 20);
  if (i == 20) DEBUGPRINTLN1("Pass loaded");
  EEPROM.end();
}

void printMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  DEBUGPRINT1("MAC: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(mac[i], HEX);
    Serial.print(":");
  }
  Serial.println(mac[5], HEX);
}


bool iotUpdater(String server, String url, String firmware, bool immediately, bool debug) {
  bool retValue = true;

  DEBUGPRINTLN1("");
  DEBUGPRINT1("updateSpaces ");
  DEBUGPRINTLN1(rtcMem.updateSpaces);

  if (rtcMem.updateSpaces >= UPDATEINTERVAL || immediately) {
    if (debug) {
      printMacAddress();
      DEBUGPRINT1("IP = ");
      DEBUGPRINTLN1(WiFi.localIP());
      DEBUGPRINT1("Update_server ");
      DEBUGPRINTLN1(server);
      DEBUGPRINT1("UPDATE_URL ");
      DEBUGPRINTLN1(url);
      DEBUGPRINT1("FIRMWARE_VERSION ");
      DEBUGPRINTLN1(firmware);
      DEBUGPRINTLN1("Updating...");
    }
    t_httpUpdate_return ret = ESPhttpUpdate.update(server, 80, url, firmware);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        retValue = false;
        if (debug) Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        DEBUGPRINTLN1();
        break;

      case HTTP_UPDATE_NO_UPDATES:
        if (debug) DEBUGPRINTLN1("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        if (debug) DEBUGPRINTLN1("HTTP_UPDATE_OK");
        break;
    }
    DEBUGPRINTLN1("Zero updateSpaces");
    rtcMem.updateSpaces = 0;  
  } else {
    rtcMem.updateSpaces = ++rtcMem.updateSpaces; // not yet an update
  }
  writeRTCmem();
  return retValue;
}


