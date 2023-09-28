#include <Arduino.h>
#include <string>
#include <vector>
#include "pinout.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <FS.h>
#include <FFat.h>
#include <TaskScheduler.h>
#include <Adafruit_GFX.h>
//#include <U8g2_for_Adafruit_GFX.h>
//#include <ILI9488.h>
//#include <Fonts/FreeSansBold9pt7b.h>
#include <Wifi.h>
#include <WiFiUdp.h>
#include <ClickEncoder.h>
#include <menu.h>
#include <menuIO/adafruitGfxOut.h>
#include <menuIO/TFT_eSPIOut.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include <menuIO/chainStream.h>
#include <Fonts/Arial14.h>

#include <plugin/SdFatMenu.h>
//enable this include if using esp8266
//#include <menuIO/esp8266Out.h>

struct  Config {
  char ssid[20];
  char pass[20];
  char ipaddr[47];
  int udpport = 1234;
  int conType;
  int fileStorage;
  int romCheck;
};


#define MAX_DEPTH 5
#define TFT_GRAY RGB565(128,128,128)
#define textScale 1.5
#define FORMAT_SPIFFS_IF_FAILED true
#define FORMAT_FFAT true


const colorDef<uint16_t> colors[6] MEMMODE={
  {{(uint16_t)TFT_BLACK,(uint16_t)TFT_BLACK}, {(uint16_t)TFT_BLACK, (uint16_t)TFT_BLUE,  (uint16_t)TFT_BLUE}},//bgColor
  {{(uint16_t)TFT_GRAY, (uint16_t)TFT_GRAY},  {(uint16_t)TFT_WHITE, (uint16_t)TFT_WHITE, (uint16_t)TFT_WHITE}},//fgColor
  {{(uint16_t)TFT_WHITE,(uint16_t)TFT_BLACK}, {(uint16_t)TFT_YELLOW,(uint16_t)TFT_YELLOW,(uint16_t)TFT_RED}},//valColor
  {{(uint16_t)TFT_WHITE,(uint16_t)TFT_BLACK}, {(uint16_t)TFT_WHITE, (uint16_t)TFT_YELLOW,(uint16_t)TFT_YELLOW}},//unitColor
  {{(uint16_t)TFT_WHITE,(uint16_t)TFT_GRAY},  {(uint16_t)TFT_BLACK, (uint16_t)TFT_BLUE,  (uint16_t)TFT_WHITE}},//cursorColor
  {{(uint16_t)TFT_WHITE,(uint16_t)TFT_YELLOW},{(uint16_t)TFT_BLUE,  (uint16_t)TFT_RED,   (uint16_t)TFT_RED}},//titleColor
};


using namespace Menu;

const char* constMEM hexDigit MEMMODE="0123456789ABCDEF";
const char* constMEM hexNr[] MEMMODE={"0","x",hexDigit,hexDigit};
const char* constMEM alphaNum MEMMODE=" 0123456789.AÁBCDEÉFGHIÍJKLMNOÓÖŐPQRSTUÚÜŰVWXYZaábcdeéfghiíjklmnoóöőpqrstuúüűvwxyz,\\|!\"#$%&/()=?~*^+-{}[]€";
const char* constMEM alphaNumMask[] MEMMODE={alphaNum};

// Előre deklarált függvények és eljárások
result zZz() {Serial.println("zZz");return proceed;}
result showEvent(eventMask e,navNode& nav,prompt& item);
result doAlert(eventMask e, prompt &item);
result alert(menuOut& o,idleEvent e);
result idle(menuOut& o,idleEvent e);
result actRomRead(eventMask e, prompt& item);
result actRomWrite(eventMask e, prompt& item);
result actRomErase(eventMask e, prompt& item);
result actRomEraseFast(eventMask e, prompt& item);
result actRomCheck(eventMask e, prompt& item);
result actSelConnect(eventMask e, prompt& item);
result actSaveSettings(eventMask e, prompt& item);
result actLoadSettings(eventMask e, prompt& item);
result actListFiles(eventMask e, prompt& item);
result actHttpConnect(eventMask e, prompt &item);
result filePick(eventMask event, navNode& nav, prompt &item);
result actRestart(eventMask e, prompt& item);
result actFontChange(eventMask e,navNode& nav ,prompt& item);
void listSPIFFS();
void setDefConfig();
void timerIsr();
void parseCommand(char *command, IPAddress remoteIP, uint16_t remotePort);
void saveConfig();
void findFonts();

int sendAddress(uint16_t address);


// Globális változók
//ILI9488 tft = ILI9488(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);
TFT_eSPI tft2 = TFT_eSPI();

//U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
float test=55;
int ledCtrl=LOW;
int selTest=0;
int chooseTest=-1;
int selRomType=0;
int selWriteVoltage=0;
int romCheck = 0;
char fname[] = "                   ";
char buf1[]="0x11";
char name[]="                                                  ";
char ipaddr[47] = "                                              ";
int fileStorage = 0;
//ClickEncoder *encoder;
//ClickEncoderStream encStream(*encoder,1);
//ESP8266Timer ITimer;
Config Myconfig;
//Task t1(1000, TASK_FOREVER, &timerIsr);
WiFiUDP udp;
//FFat sd;
String fonts[30] = {
  "Arial14"
  ,"Arial-BoldMT-14"
  ,"Arial-Black-14"
  ,"Calibri-Bold-14"
  ,"Georgia-Bold-14"
  ,"Impact-14"
  ,"InkFree-14"
  ,"LeelawadeeUI-14"
  ,"MVBoli-14"
};

SDMenuT<CachedFSO<fs::F_Fat,32>> filePickMenu(FFat,"SD Card","/",filePick,enterEvent);

// Menük
SELECT(selRomType,mnuRomtype,"Rom típus :",doNothing,noEvent,noStyle
  ,VALUE("Retro(27.../17...)",0,doNothing,noEvent)
  ,VALUE("Modern",1,doNothing,noEvent)
  ,VALUE("I2C",2,doNothing,noEvent)
  ,VALUE("SPI",3,doNothing,noEvent)
);
SELECT(selWriteVoltage,mnuVoltage,"Égető feszültség :",doNothing,noEvent,noStyle
  ,VALUE("5V",0,doNothing,noEvent)
  ,VALUE("6V",1,doNothing,noEvent)
  ,VALUE("12V",2,doNothing,noEvent)
  ,VALUE("21V",3,doNothing,noEvent)
  ,VALUE("25V",4,doNothing,noEvent)
);
TOGGLE(Myconfig.romCheck,mnuRomCheck,"Ellenőrzés írás után :",doNothing,noEvent,noStyle//,doExit,enterEvent,noStyle
  ,VALUE("BE",1,doNothing,noEvent)
  ,VALUE("KI",0,doNothing,noEvent)
);
int selFont=0;
SELECT(selFont,mnuFont,"Betűtípus :",actFontChange,exitEvent,noStyle
  ,VALUE("Arial14",0,doNothing,noEvent)
  ,VALUE("Arial-BoldMT-14",1,doNothing,noEvent)
  ,VALUE("Arial-Black-14",2,doNothing,noEvent)
  ,VALUE("Calibri-Bold-14",3,doNothing,noEvent)
  ,VALUE("Georgia-Bold-14",4,doNothing,noEvent)
  ,VALUE("Impact-14",5,doNothing,noEvent)
  ,VALUE("InkFree-14",6,doNothing,noEvent)
  ,VALUE("LeelawadeeUI-14",7,doNothing,noEvent)
  ,VALUE("MVBoli-14",8,doNothing,noEvent)
);


MENU(mnuEprom,"EPROM műveletek",showEvent,anyEvent,noStyle
  ,SUBMENU(mnuRomtype)
  ,SUBMENU(mnuVoltage)
  ,SUBMENU(mnuRomCheck)
  ,EDIT("Fájl név :",fname,alphaNumMask,doNothing,noEvent,noStyle)
  ,OP("Rom kiolvasása",actRomRead,enterEvent)
  ,OP("Rom írása",actRomWrite,enterEvent)
  ,OP("Rom törlése",actRomErase,enterEvent)
  ,OP("Rom törlése (gyors)",actRomEraseFast,enterEvent)
  ,OP("Rom ellenőrzése",actRomCheck,enterEvent)
  ,EXIT("<Vissza")
);



MENU(mnuFile,"Fájl műveletek",doNothing,noEvent,noStyle
  ,OP("Fájlok listázása",actListFiles,enterEvent)
  //,OP("Fájl kiválasztása",doNothing,enterEvent)
  ,SUBMENU(filePickMenu)
  ,OP("Fájl törlése",doNothing,enterEvent)
  ,OP("Fájl átnevezése",doNothing,enterEvent)
  ,OP("Fájl küldése",doNothing,enterEvent)
  ,OP("Fájl fogadása",doNothing,enterEvent)
  ,EXIT("<Vissza")
);

SELECT(Myconfig.conType,mnuConType,"Kapcsolat :",actSelConnect,exitEvent,noStyle
  ,VALUE("UDP",0,doNothing,noEvent)
  ,VALUE("HTTP",1,doNothing,noEvent)
  ,VALUE("USB/Serial",2,doNothing,noEvent)
);
//Kapcsolódások menü
MENU(mnuConnections,"Csatlakozások",doNothing,noEvent,noStyle
  ,SUBMENU(mnuConType)
  ,EDIT("SSID :",Myconfig.ssid,alphaNumMask,doNothing,noEvent,noStyle)
  ,EDIT("Jelszó :",Myconfig.pass,alphaNumMask,doNothing,noEvent,noStyle)
  ,EDIT("IP cím :",Myconfig.ipaddr,alphaNumMask,doNothing,noEvent,noStyle)
  ,FIELD(Myconfig.udpport,"Port :","",0,65535,1,1,doNothing,noEvent,noStyle)
  ,EXIT("<Vissza")
);
//Eprom bináros tárolása menü
SELECT(Myconfig.fileStorage,mnuStorage,"Romok tárolása :",doNothing,noEvent,noStyle
  ,VALUE("Flash",0,doNothing,noEvent)
  ,VALUE("SD kártya",1,doNothing,noEvent)
);
//Kijelző beállítása menü
MENU(mnuScreen,"Kijelző beállítása",doNothing,noEvent,noStyle
  ,SUBMENU(mnuFont)
  ,OP("Kijelző teszt",doNothing,enterEvent)
  ,OP("Kijelző beállítása",doNothing,enterEvent)
  ,EXIT("<Vissza")
);

MENU(mnuReceiveSettings,"Beállítások fogadása",doNothing,noEvent,noStyle
  ,OP("Beállítások fogadása UDP-n",doNothing,enterEvent)
  ,OP("Beállítások fogadása HTTP-n",doNothing,enterEvent)
  ,OP("Beállítások fogadása USB-n",doNothing,enterEvent)
  ,EXIT("<Vissza")
);

//Beállítások menü
MENU(mnuMySettings,"Beállítások",doNothing,noEvent,noStyle
  ,SUBMENU(mnuConnections)
  ,SUBMENU(mnuStorage)
  ,SUBMENU(mnuScreen)
  ,OP("Beállítások mentése",actSaveSettings,enterEvent)
  ,OP("Beállítások betöltése",actLoadSettings,enterEvent)
  ,OP("Beállitások alaphelyzetbe állítása",doNothing,enterEvent)
  ,SUBMENU(mnuReceiveSettings)
  ,OP("Újraindítás",actRestart,enterEvent)
  ,EXIT("<Vissza")
);


//Főmenü
MENU(mainMenu,"Főmenü",zZz,noEvent,wrapStyle
  ,SUBMENU(mnuEprom)
  ,SUBMENU(mnuFile)
  ,SUBMENU(mnuMySettings)
  ,EXIT("<Back")
);
//Kimenetek beállítása
// MENU_OUTPUTS(out,MAX_DEPTH
//   ,ADAGFX_OUT(tft2,colors,6*textScale,9*textScale,{0,0,14,8},{14,0,14,8})
//   ,SERIAL_OUT(Serial)
// );

#define GFX_WIDTH 480
#define GFX_HEIGHT 320
#define fontW 8
#define fontH 16

//define serial output device
idx_t serialTops[MAX_DEPTH]={0};
serialOut outSerial(Serial,serialTops);


const panel panels[] MEMMODE = {{0, 0, GFX_WIDTH / fontW, GFX_HEIGHT / fontH}};
navNode* nodes[sizeof(panels) / sizeof(panel)]; //navNodes to store navigation status
panelsList pList(panels, nodes, 1); //a list of panels and nodes
idx_t eSpiTops[MAX_DEPTH]={0};
TFT_eSPIOut eSpiOut(tft2,colors,eSpiTops,pList,fontW,fontH+1);
menuOut* constMEM outputs[] MEMMODE={&outSerial,&eSpiOut};//list of output devices
outputsList out(outputs,sizeof(outputs)/sizeof(menuOut*));//outputs list controller

serialIn serial(Serial);
MENU_INPUTS(in,&serial);
NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

void formatFFat() {
  if (FORMAT_FFAT) {
    Serial.println("FFat formázás...");
    if (!FFat.format()) {
      Serial.println("FFat formázás sikertelen");
      return;
    }
    Serial.println("FFat formázás sikeres");
  }
}

void linePrint(const char *text) {
  Serial.println(text);
}

void linePrint(const String text) {
  Serial.println(text);
  //u8g2Fonts.println(text);
}


void setup() {
  for (int i = 0; i < 30; i++) {
    fonts[i] = "";
  }
  Serial.begin(115200);
  while(!Serial);
  linePrint("ESP32 Eprom programmer");

  if(!FFat.begin()){
    linePrint("FFat csatolása sikertelen");
    formatFFat();
    FFat.begin();
  }
  linePrint("FFat fájlrendszer csatolva");
  findFonts();
  if (!FFat.exists("/config.json")) {
    linePrint("A konfigurációs fájl nem létezik, létrehozás...");
    File configFile = FFat.open("/config.json", "w");
    setDefConfig();
    if (!configFile) {
      linePrint("Nem sikerült megnyitni a konfigurációs fájlt írásra");
    }
    filePickMenu.begin();
    StaticJsonDocument<512> doc;
    doc["ssid"] = Myconfig.ssid;
    doc["pass"] = Myconfig.pass;
    //doc["ipaddr"] = Myconfig.ipaddr;
    doc["udpport"] = Myconfig.udpport;
    doc["conType"] = Myconfig.conType;
    doc["fileStorage"] = Myconfig.fileStorage;
    doc["romCheck"] = Myconfig.romCheck;
    serializeJson(doc, configFile);
    configFile.close();
  } else {
    StaticJsonDocument<512> doc;
    File configFile = FFat.open("/config.json", "r");
    if (!configFile) {
      linePrint("Nem sikerült megnyitni a konfigurációs fájlt olvasásra");
    }
    DeserializationError error = deserializeJson(doc, configFile);
    if (!error) { // Ezt majd vissza kell írni
      linePrint("Nem sikerült beolvasni a konfigurációs fájlt");
      setDefConfig();
      saveConfig();
    } else {
      strcpy(Myconfig.ssid,doc["ssid"]);
      strcpy(Myconfig.pass,doc["pass"]);
      Myconfig.udpport = doc["udpport"];
      Myconfig.conType = doc["conType"];
      Myconfig.fileStorage = doc["fileStorage"];
      Myconfig.romCheck = doc["romCheck"];
    }
  }
  linePrint("Konfigurációs fájl beolvasva");
  if (Myconfig.conType == 0 || Myconfig.conType == 1) {
    Serial.println(Myconfig.ssid);
    WiFi.begin(Myconfig.ssid, Myconfig.pass);
    Serial.print("Csatlakozás a ");
    Serial.print(Myconfig.ssid);
    Serial.println(" hálózathoz");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      i++;
      if (i > 20) {
        Serial.println("Nem sikerült csatlakozni a hálózathoz");
        break;
      }
    }
    Serial.println("");
    Serial.println("WiFi kapcsolat létrejött");
    Serial.print("IP cím: ");
    Serial.println(WiFi.localIP());
    char ip[47];
    WiFi.localIP().toString().toCharArray(ip, 47);
    strcpy(Myconfig.ipaddr, ip);

  }
  if (Myconfig.conType == 0) {
    Serial.println("UDP kapcsolat");
  } else if (Myconfig.conType == 1) {
    Serial.println("HTTP kapcsolat");
  } else if (Myconfig.conType == 2) {
    Serial.println("USB/Serial kapcsolat");
  }
  if (Myconfig.conType == 0) {
    udp.begin(Myconfig.udpport);
  }
  //encoder = new ClickEncoder(ENCODER_PINA, ENCODER_PINB, ENCODER_BTN, 4);

  delay(500);
  tft2.begin();
  tft2.loadFont("Arial14",FFat);
  tft2.setTextSize(textScale);
  //tft2.setFreeFont(&Arial14);
  
  tft2.setRotation(1);
  tft2.fillScreen(TFT_BLACK);
  tft2.setTextColor(TFT_RED, TFT_BLACK);
  tft2.println("ESP32 Eprom programmer");
  tft2.setTextSize(textScale);

  tft2.setCursor(0, 0);
  nav.timeOut=70;
  nav.idleTask=idle;//point a function to be used when menu is suspended
}

void loop() {
  IPAddress remoteIP;
  uint16_t remotePort;
  nav.poll();
  #ifdef ESP8266
  //digitalWrite(LEDPIN, ledCtrl);
  #endif
  if (Myconfig.conType == 0) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      char packetBuffer[255];
      int len = udp.read(packetBuffer, 255);
      if (len > 0) {
        packetBuffer[len] = 0;
      }
      remoteIP = udp.remoteIP();
      remotePort = udp.remotePort();
      parseCommand(packetBuffer, remoteIP, remotePort);
    }
  }
  delay(100);//simulate a delay when other tasks are done
}

// Függvények és eljárások

result showEvent(eventMask e,navNode& nav,prompt& item) {
  Serial.print("event: ");
  Serial.println(e);
  return proceed;
}

result actRestart(eventMask e, prompt& item) {
  ESP.restart();
  return proceed;
}



result alert(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.setCursor(0,0);
    o.print("alert test");
    o.setCursor(0,1);
    o.print("press [select]");
    o.setCursor(0,2);
    o.print("to continue...");
  }
  return proceed;
}

result doAlert(eventMask e, prompt &item) {
  nav.idleOn(alert);
  return proceed;
}

result idle(menuOut &o, idleEvent e) {
  // o.clear();
  switch(e) {
    case idleStart:o.println("suspending menu!");break;
    case idling:o.println("suspended...");break;
    case idleEnd:o.println("resuming menu.");
      nav.reset();
      break;
  }
  return proceed;
}

result romReadIdle(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.setCursor(0,0);
    o.print("reading from ROM");
    o.setCursor(0,1);
    o.print("press [select]");
    o.setCursor(0,2);
    o.print("to continue...");
  }
  return proceed;
}

result actRomRead(eventMask e,prompt& item) {
  nav.idleOn(romReadIdle);
  return proceed;
}
result actRomWrite(eventMask e,prompt& item) {
  nav.idleOn(romReadIdle);
  return proceed;
}
result actRomErase(eventMask e,prompt& item) {
  nav.idleOn(romReadIdle);
  return proceed;
}
result actRomClear(eventMask e,prompt& item) {
  nav.idleOn(romReadIdle);
  return proceed;
}
result actRomEraseFast(eventMask e,prompt& item) {
  nav.idleOn(romReadIdle);
  return proceed;
}
result actRomCheck(eventMask e,prompt& item) {
  nav.idleOn(romReadIdle);
  return proceed;
}

result actSelConnect(eventMask e,prompt& item) {
  if  (Myconfig.conType > 0) {
    mnuConnections[1].enabled = disabledStatus;
    mnuConnections[2].enabled = disabledStatus;
  } else {
    mnuConnections[1].enabled = enabledStatus;
    mnuConnections[2].enabled = enabledStatus;
  }
  return proceed;
}

result idleSaveSettings(menuOut& o,idleEvent e) {
  if (e==idling) {
    saveConfig();
    o.setCursor(0,0);
    o.print("saving settings");
    o.setCursor(0,1);
    o.print("press [select]");
    o.setCursor(0,2);
    o.print("to continue...");
  }
  return proceed;
}

result actSaveSettings(eventMask e,prompt& item) {
  nav.idleOn(idleSaveSettings);
  return proceed;
}

result idleLoadSettings(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.setCursor(0,0);
    o.print("loading settings");
    o.setCursor(0,1);
    o.print("press [select]");
    o.setCursor(0,2);
    o.print("to continue...");
  }
  return proceed;
}

result actLoadSettings(eventMask e,prompt& item) {
  nav.idleOn(idleLoadSettings);
  return proceed;
}

result idleListFiles(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.clear();
    
    o.setCursor(0,0);
    o.println("listing files");
    o.println("on SPIFFS:");
    o.println("-------------");
    File root = FFat.open("/");
    if (!root) {
      o.println("Failed to open directory");
      delay(2000);
      return proceed;
    } else {
      while (File file = root.openNextFile()) {
        o.println(file.name());
        file.close();
      }
    }
    o.print("press [select]");
    o.println("to continue...");
  }
  return proceed;
}
result actListFiles(eventMask e,prompt& item) {
  nav.idleOn(idleListFiles);
  return proceed;
}

int sendAddress(uint16_t address) {
  // Loop through the 12 bits from MSB to LSB
  for (int i = 11; i >= 0; i--) {
    // Write a HIGH pulse to the clock pin
    digitalWrite(ADDR_OUT, HIGH);
    // Delay for 1 microsecond
    delayMicroseconds(1);
    // Write the bit value to the data pin
    digitalWrite(ADDR_OUT, address & (1 << i));
    // Delay for 1 microsecond
    delayMicroseconds(1);
    // Write a LOW pulse to the clock pin
    digitalWrite(ADDR_OUT, LOW);
    // Delay for 1 microsecond
    delayMicroseconds(1);
    
  }
  return 0;
}

int writeParallelEprom(uint16_t address, uint8_t data) {
  int result = 0;
  // Set the address pins
  sendAddress(address);
  // Loop through the 8 bits from MSB to LSB
  for (int i = 7; i >= 0; i--) {
    digitalWrite(DATA_OUT, HIGH);
    delayMicroseconds(1);
    digitalWrite(DATA_OUT, data & (1 << i));
    delayMicroseconds(1);
    digitalWrite(DATA_OUT, LOW);
    delayMicroseconds(1);
  }
  digitalWrite(DATA_OUT, HIGH);
  delayMicroseconds(1);
  digitalWrite(DATA_OUT, LOW);
  delayMicroseconds(1);
  return result;
}

void listSPIFFS() {
  Serial.println("Listing SPIFFS files");
  File root = FFat.open("/");
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  if (!file) {
    Serial.println("No files found");
    return;
  }
  while (file) {
    Serial.print("  FILE: ");
    Serial.print(file.name());
    Serial.print("  SIZE: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
  Serial.flush();
}

void setDefConfig() {
  strcpy(Myconfig.ssid,"DIGI_7fe918");
  strcpy(Myconfig.pass,"ddcc8016");
  strcpy(Myconfig.ipaddr,"");
  Myconfig.udpport = 1234;
  Myconfig.conType = 0;
  Myconfig.fileStorage = 0;
  Myconfig.romCheck = 0;
}

//TODO: HTTP kapcsolat megírása
result idleHttpConnect(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.setCursor(0,0);
    o.println("A modul fejlesztés alatt....");
    o.print("press [select]");
    o.setCursor(0,3);
    o.print("to continue...");
  }
  return proceed;
}

result filePick(eventMask event, navNode& nav, prompt &item) {
  // switch(event) {//for now events are filtered only for enter, so we dont need this checking
  //   case enterCmd:
      if (nav.root->navFocus==(navTarget*)&filePickMenu) {
        Serial.println();
        Serial.print("selected file:");
        Serial.println(filePickMenu.selectedFile);
        Serial.print("from folder:");
        Serial.println(filePickMenu.selectedFolder);
      }
  //     break;
  // }
  return proceed;
}


void parseCommand(char *command, IPAddress remoteIP, uint16_t remotePort) {
  //TODO: UDP parancsok feldolgozása,és a fájlok fogadása. Fejlesztés alatt
  //Parancsok: PUTFILE, GETFILE, LISTFILES, GETCONFIG, SETCONFIG, GETROM, SETROM, GETSTATUS, SETSTATUS, GETROMTYPE, SETROMTYPE, GETVOLTAGE, SETVOLTAGE, GETROMCHECK, SETROMCHECK, GETFILESTORAGE, SETFILESTORAGE, GETIP, SETIP, GETPORT, SETPORT
  //Parancsok és paramétereinek szétválasztása
  char *pch;
  std::vector<std::string> params;
  pch = strtok(command," ");
  while (pch != NULL) {
    params.push_back(pch);
    pch = strtok(NULL," ");
  }
  if (strcmp(params[0].c_str(),"PUTFILE") == 0) {
    Serial.println("Fájl fogadása");
    if (params.size() == 3) {
      Serial.println(params[0].c_str());
      Serial.println(params[1].c_str());
      Serial.println(params[2].c_str());
      Serial.println(remoteIP);
      Serial.println(remotePort);
    } else {
      Serial.println("Hibás paraméterek");
    }
  } else if (strcmp(params[0].c_str(),"GETFILE") == 0) {
    Serial.println("Fájl küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"LISTFILES") == 0) {
    Serial.println("Fájlok listázása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETCONFIG") == 0) {
    Serial.println("Konfiguráció küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETCONFIG") == 0) {
    Serial.println("Konfiguráció fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROM") == 0) {
    Serial.println("Rom küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROM") == 0) {
    Serial.println("Rom fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETSTATUS") == 0) {
    Serial.println("Státusz küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETSTATUS") == 0) {
    Serial.println("Státusz fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMTYPE") == 0) {
    Serial.println("Rom típus küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMTYPE") == 0) {
    Serial.println("Rom típus fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETVOLTAGE") == 0) {
    Serial.println("Feszültség küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETVOLTAGE") == 0) {
    Serial.println("Feszültség fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECK") == 0) {
    Serial.println("Rom ellenőrzés küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECK") == 0) {
    Serial.println("Rom ellenőrzés fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETFILESTORAGE") == 0) {
    Serial.println("Fájl tárolás küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETFILESTORAGE") == 0) {
    Serial.println("Fájl tárolás fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETIP") == 0) {
    Serial.println("IP cím küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETIP") == 0) {
    Serial.println("IP cím fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETPORT") == 0) {
    Serial.println("Port küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETPORT") == 0) {
    Serial.println("Port fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETSSID") == 0) {
    Serial.println("SSID küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETSSID") == 0) {
    Serial.println("SSID fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETPASS") == 0) {
    Serial.println("Jelszó küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETPASS") == 0) {
    Serial.println("Jelszó fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETCONTYPE") == 0) {
    Serial.println("Kapcsolat típus küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETCONTYPE") == 0) {
    Serial.println("Kapcsolat típus fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMSIZE") == 0) {
    Serial.println("Rom méret küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMSIZE") == 0) {
    Serial.println("Rom méret fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMNAME") == 0) {
    Serial.println("Rom név küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMNAME") == 0) {
    Serial.println("Rom név fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMDATE") == 0) {
    Serial.println("Rom dátum küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMDATE") == 0) {
    Serial.println("Rom dátum fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMTIME") == 0) {
    Serial.println("Rom idő küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMTIME") == 0) {
    Serial.println("Rom idő fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUM") == 0) {
    Serial.println("Rom ellenőrző összeg küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUM") == 0) {
    Serial.println("Rom ellenőrző összeg fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMTYPE") == 0) {
    Serial.println("Rom ellenőrző összeg típus küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUMTYPE") == 0) {
    Serial.println("Rom ellenőrző összeg típus fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMSIZE") == 0) {
    Serial.println("Rom ellenőrző összeg méret küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUMSIZE") == 0) {
    Serial.println("Rom ellenőrző összeg méret fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMADDRESS") == 0) {
    Serial.println("Rom ellenőrző összeg cím küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUMADDRESS") == 0) {
    Serial.println("Rom ellenőrző összeg cím fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMDATA") == 0) {
    Serial.println("Rom ellenőrző összeg adat küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUMDATA") == 0) {
    Serial.println("Rom ellenőrző összeg adat fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMRESULT") == 0) {
    Serial.println("Rom ellenőrző összeg eredmény küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUMRESULT") == 0) {
    Serial.println("Rom ellenőrző összeg eredmény fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMRESULTTYPE") == 0) {
    Serial.println("Rom ellenőrző összeg eredmény típus küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUMRESULTTYPE") == 0) {
    Serial.println("Rom ellenőrző összeg eredmény típus fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMRESULTSIZE") == 0) {
    Serial.println("Rom ellenőrző összeg eredmény méret küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"SETROMCHECKSUMRESULTSIZE") == 0) {
    Serial.println("Rom ellenőrző összeg eredmény méret fogadása");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
    Serial.println(remotePort);
  } else if (strcmp(params[0].c_str(),"GETROMCHECKSUMRESULTADDRESS") == 0) {
    Serial.println("Rom ellenőrző összeg eredmény cím küldése");
    Serial.println(params[0].c_str());
    Serial.println(remoteIP);
  }
}

result actHttpConnect(eventMask e,prompt& item) {
  nav.idleOn(idleHttpConnect);
  return proceed;
}

void saveConfig() {
  File configFile = FFat.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Nem sikerült megnyitni a konfigurációs fájlt írásra");
  }
  StaticJsonDocument<512> doc;
  doc["ssid"] = Myconfig.ssid;
  doc["pass"] = Myconfig.pass;
  doc["udpport"] = Myconfig.udpport;
  doc["conType"] = Myconfig.conType;
  doc["fileStorage"] = Myconfig.fileStorage;
  doc["romCheck"] = Myconfig.romCheck;
  serializeJson(doc, configFile);
  configFile.close();
}


// Interrupt rutin
void timerIsr() {
  //encoder->service();
}

void findFonts() {
  File dr = FFat.open("/");
  int i = 0;
  if (!dr) {
      linePrint("Nem található betűtípus");
  } else {
    while (true) {
      File entry =  dr.openNextFile();
      if (!entry) {
        break;
      }
      String name = entry.name();
      if (name.endsWith(".vlw")) {
        name.remove(name.length() - 4);
        fonts[i++] = name;
        linePrint(name);
      }
      
      entry.close();
    }
    // i--;
    // prompt* fontList[i];
    // for (int j = 0; j < i-1; j++) {
    //   fontList[j] = new menuValue<int>(fonts[j].c_str(),j);
    // }
    // //mnuFont = new menu("Betűtípus",fontList,i);
    // //choose<int>& durMenu =*new choose<int>("Duration",duration,sizeof(durData)/sizeof(prompt*),durData);
    // mnuFont = *new Menu::select<int>("Betűtípus : ",selFont
    // ,sizeof(fontList)/sizeof(prompt*),fontList,(Menu::callback)actFontChange,anyEvent);
    dr.close();
  }
}

result actFontChange(eventMask e,navNode& nav ,prompt& item) {
  if (e==exitEvent) {
    Serial.println("");
    Serial.print("selected font: " + fonts[selFont]);
    //delay(5000);
    //item.
    tft2.unloadFont();
    tft2.loadFont(fonts[selFont],FFat);
    tft2.setTextSize(textScale);
    tft2.fillScreen(TFT_BLACK);
    tft2.setTextColor(TFT_RED, TFT_BLACK);
    tft2.println("ESP32 Eprom programmer");
    tft2.setTextSize(textScale);
    tft2.setCursor(0, 0);
    nav.reset();
    // nav.timeOut=70;
    // nav.idleTask=idle;//point a function to be used when menu is suspended
  }
  return proceed;
}
