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
#include <Wifi.h>
#include <WiFiUdp.h>
#include <menu.h>
#include <menuIO/adafruitGfxOut.h>
#include <menuIO/TFT_eSPIOut.h>
#include <menuIO/analogAxisIn.h>
#include <menuIO/AltkeyIn.h>
//#include <menuIO/clickEncoderIn.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include <menuIO/chainStream.h>
#include <plugin/SdFatMenu.h>
#include <TFT_eFEX.h>   // Include the extension graphics functions library
//#include <AnalogJoystick.h>
#include "TFT_Menu.h"
//#include "filemenu.h"
struct  Config {
  char ssid[20];
  char pass[20];
  char ipaddr[47];
  int udpport = 1234;
  int conType;
  int fileStorage;
  int romCheck;
  int scrFont;
  int scrTajol=0;
  float scale = 1.5;
  int splash = 0;
};


#define MAX_DEPTH 5
#define TFT_GRAY RGB565(128,128,128)
#define textScale 1.5
#define FORMAT_SPIFFS_IF_FAILED true
#define FORMAT_FFAT true
#define JOY_X 36
#define JOY_Y 39
#define JOY_BTN 12

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
result actScrTajol(eventMask e,navNode& nav ,prompt& item);
result actChangeScale(eventMask e,navNode& nav ,prompt& item);
result actListSettings(eventMask e, navNode& nav, prompt& item);
result actScreenTest(eventMask e, navNode& nav, prompt& item);
result actSplash(eventMask e, navNode& nav, prompt& item);
void setDefConfig();
void timerIsr();
void parseCommand(char *command, IPAddress remoteIP, uint16_t remotePort);
void saveConfig();
int sendAddress(uint16_t address);
void getFile(String fname, uint32_t fsize);
void fillItems();

analogAxis<JOY_Y,10,false> ay;

#define joyBtn 12

AnalogJoystick mJoy(JOY_X,JOY_Y,joyBtn,2250);

keyMap btnsMap[]={{-joyBtn,defaultNavCodes[enterCmd].ch}};//negative pin numbers use internal pull-up, this is on when low
keyIn<1> btns(btnsMap);// 1 is the number of keys


// Globális változók
TFT_eSPI tft = TFT_eSPI();
TFT_eFEX  fex = TFT_eFEX(&tft);    // Create TFT_eFX object "efx" with pointer to "tft" object
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
Config Myconfig;
WiFiUDP udp;
TFT_MENU xMenu(tft,mJoy,1);
std::vector<MENU> xMenuItems;

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

String bitmaps[50] = {
  "/Betti2.bmp"
};



SDMenuT<CachedFSO<fs::F_Fat,32>> filePickMenu(FFat,"FFAT partíció","/",filePick,enterEvent);

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


MENU(mnuEprom,"EPROM műveletek",doNothing,noEvent,noStyle
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

SELECT(Myconfig.scrTajol,mnuScrTajol,"Kijelző tájolása :",actScrTajol,exitEvent,noStyle
  ,VALUE("Függőleges",0,doNothing,noEvent)
  ,VALUE("Vízszintes",1,doNothing,noEvent)
  ,VALUE("Függőleges tükrözve",2,doNothing,noEvent)
  ,VALUE("Vízszintes tükrözve",3,doNothing,noEvent)
);

SELECT(Myconfig.splash,mnuSplash,"Bejelentkező kép",actSplash,exitEvent,noStyle
  ,VALUE("Betti2.bmp",0,doNothing,noEvent)
  ,VALUE("Betti3.bmp",1,doNothing,noEvent)
);

//Kijelző beállítása menü
MENU(mnuScreen,"Kijelző beállítása",doNothing,noEvent,noStyle
  ,SUBMENU(mnuFont)
  ,SUBMENU(mnuScrTajol)
  ,FIELD(Myconfig.scale,"Kijelző mérete :","",1,3,0.1,1,actChangeScale,exitEvent,noStyle)
  ,OP("Kijelző teszt",actScreenTest,enterEvent)
  ,OP("Kijelző beállítása",doNothing,enterEvent)
  ,EXIT("<Vissza")
);

MENU(mnuReceiveSettings,"Beállítások fogadása",doNothing,noEvent,noStyle
  ,OP("Beállítások fogadása UDP-n",doNothing,enterEvent)
  ,OP("Beállítások fogadása HTTP-n",doNothing,enterEvent)
  ,OP("Beállítások fogadása USB-n",doNothing,enterEvent)
  ,OP("Beállítások küldése USB-n", actListSettings, enterEvent)
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
TFT_eSPIOut eSpiOut(tft,colors,eSpiTops,pList,fontW,fontH+1);
menuOut* constMEM outputs[] MEMMODE={&outSerial,&eSpiOut};//list of output devices
outputsList out(outputs,sizeof(outputs)/sizeof(menuOut*));//outputs list controller

serialIn serial(Serial);
MENU_INPUTS(in,&ay,&btns,&serial);
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
  tft.println(text);
}

void linePrint(const String text) {
  Serial.println(text);
  tft.println(text);
}

void charPrint(const char *text) {
  Serial.print(text);
  tft.print(text);
}


void setup() {
  Serial.begin(115200);
  while(!Serial);
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  linePrint("ESP32 Eprom programmer");
  linePrint("Joystick kalibrálása");
  uint32_t iKalVal = 0;
  for (int i = 0; i < 50; i++) {
    charPrint(".");
    iKalVal += analogRead(JOY_Y);
    delay(100);
  }
  ay.setCalibration(iKalVal / 50);
  linePrint("Kész");
  if(!FFat.begin()){
    linePrint("FFat csatolása sikertelen");
    formatFFat();
    FFat.begin();
  }
  linePrint("FFat fájlrendszer csatolva");
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
    doc["udpport"] = Myconfig.udpport;
    doc["conType"] = Myconfig.conType;
    doc["fileStorage"] = Myconfig.fileStorage;
    doc["romCheck"] = Myconfig.romCheck;
    doc["scrFont"] = Myconfig.scrFont;
    doc["scrTajol"] = Myconfig.scrTajol;
    doc["splash"] = Myconfig.splash;
    serializeJson(doc, configFile);
    configFile.close();
  } else {
    StaticJsonDocument<512> doc;
    File configFile = FFat.open("/config.json", "r");
    if (!configFile) {
      linePrint("Nem sikerült megnyitni a konfigurációs fájlt olvasásra");
    }
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) { // Ezt majd vissza kell írni
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
      Myconfig.scrFont = doc["scrFont"];
      selFont = Myconfig.scrFont;
      Myconfig.scrTajol = doc["scrTajol"];
      Myconfig.splash = doc["splash"];
    }
  }
  linePrint("Konfigurációs fájl beolvasva");
  if (Myconfig.conType == 0 || Myconfig.conType == 1) {
    linePrint(Myconfig.ssid);
    WiFi.begin(Myconfig.ssid, Myconfig.pass);
    linePrint("Csatlakozás a ");
    linePrint(Myconfig.ssid);
    linePrint(" hálózathoz");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      charPrint(".");
      i++;
      if (i > 20) {
        linePrint("Nem sikerült csatlakozni a hálózathoz");
        break;
      }
    }
    linePrint("");
    linePrint("WiFi kapcsolat létrejött");
    charPrint("IP cím: ");
    linePrint(WiFi.localIP().toString());
    char ip[47];
    WiFi.localIP().toString().toCharArray(ip, 47);
    strcpy(Myconfig.ipaddr, ip);

  }
  if (Myconfig.conType == 0) {
    linePrint("UDP kapcsolat");
  } else if (Myconfig.conType == 1) {
    linePrint("HTTP kapcsolat");
  } else if (Myconfig.conType == 2) {
    linePrint("USB/Serial kapcsolat");
  }
  if (Myconfig.conType == 0) {
    udp.begin(Myconfig.udpport);
  }

  delay(500);
  //fex.drawJpg(Betti1, sizeof(Betti1), 0, 0);
  fex.drawBmp(FFat,"/Betti2.bmp",0,0);
  //fex.drawJpgFile(FFat, "/Betti2.jpg", 0, 0);
  
  delay(5000);
  tft.loadFont(fonts[selFont],FFat);
  tft.setTextSize(textScale);
  
  tft.setRotation(Myconfig.scrTajol);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.println("ESP32 Eprom programmer");
  tft.setTextSize(textScale);

  tft.setCursor(0, 0);
  nav.timeOut=7000;
  nav.idleTask=idle;//point a function to be used when menu is suspended
}

void loop() {
  IPAddress remoteIP;
  uint16_t remotePort;
  nav.poll();
  int kp = digitalRead(joyBtn);
  if (kp) Serial.println("KKKKKK");
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
//******************************
//*Függvények és eljárások     *
//******************************
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
    case idleStart:
      o.clear();
      o.setCursor(0,0);
      o.println("Menü felfüggesztése...");
      break;
    case idling:o.println("felfüggesztve...");break;
    case idleEnd:o.println("Menü folytatása.");
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
    MENU mymenu[] = {
      {"Teszt Menü :",           0 },
      {"Teszt 1",                1 },
      {"Teszt 2",                2 },
      {"Valami",                 3 }
    };
    TFT_MENU menu(tft,mJoy,1);
    int mm = menu.show(mymenu,1);
    o.println(mymenu[mm].option);
  }
  return proceed;
}
result actListFiles(eventMask e,prompt& item) {
  nav.idleOn(idleListFiles);
  return proceed;
}
/***************************************************************************************
** Function name:           sendAddress
** Description:             Írandó cím az EPROM-ba
***************************************************************************************/
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
/***************************************************************************************
** Function name:           writeParallelEprom
** Description:             Párhuzamos programozásu Eprom írása
***************************************************************************************/
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

/***************************************************************************************
** Function name:           setDefConfig
** Description:             Alapértelmezett paraméterek beállítása
***************************************************************************************/
void setDefConfig() {
  strcpy(Myconfig.ssid,"DIGI_7fe918");
  //strcpy(Myconfig.ssid,"Wifi_neve");
  strcpy(Myconfig.pass,"ddcc8016");
  //strcpy(Myconfig.pass,"jelszó");
  strcpy(Myconfig.ipaddr,"");
  Myconfig.udpport = 1234;
  Myconfig.conType = 0;
  Myconfig.fileStorage = 0;
  Myconfig.romCheck = 0;
  Myconfig.scrFont = 0;
  Myconfig.scrTajol = 0;
  Myconfig.scale = 1.5;
  Myconfig.splash = 0;
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

/***************************************************************************************
** Function name:           parseCommand
** Description:             UDP parancsok feldolgozása
***************************************************************************************/
void parseCommand(char *command, IPAddress remoteIP, uint16_t remotePort) {
  //TODO: UDP parancsok feldolgozása,és a fájlok fogadása. Fejlesztés alatt
  //Parancsok: PUTFILE, GETFILE, LISTFILES, GETCONFIG, SETCONFIG, GETROM, SETROM, 
  //GETSTATUS, SETSTATUS, GETROMTYPE, SETROMTYPE, GETVOLTAGE, SETVOLTAGE, GETROMCHECK, 
  //SETROMCHECK, GETFILESTORAGE, SETFILESTORAGE, GETIP, SETIP, GETPORT, SETPORT
  //Parancsok és paramétereinek szétválasztása
  char *pch;
  std::vector<std::string> params;
  pch = strtok(command,"|");
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
/***************************************************************************************
** Function name:           saveConfig
** Description:             Beállítások mentése
***************************************************************************************/
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
  doc["scrFont"] = Myconfig.scrFont;
  doc["scrTajol"] = Myconfig.scrTajol;
  doc["scale"] = Myconfig.scale;
  doc["splash"] = Myconfig.splash;
  serializeJson(doc, configFile);
  configFile.close();
}


// Interrupt rutin
void timerIsr() {
  //encoder->service();
}


result actFontChange(eventMask e,navNode& nav ,prompt& item) {
  if (e==exitEvent) {
    tft.unloadFont();
    tft.loadFont(fonts[selFont],FFat);
    Myconfig.scrFont = selFont;
    tft.setTextSize(textScale);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("ESP32 Eprom programmer");
    tft.setTextSize(textScale);
    tft.setCursor(0, 0);
    nav.reset();
  }
  return proceed;
}

result actScrTajol(eventMask e,navNode& nav ,prompt& item) {
  if (e==exitEvent) {
    tft.setRotation(Myconfig.scrTajol);
    tft.setTextSize(textScale);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("ESP32 Eprom programmer");
    tft.setTextSize(textScale);
    tft.setCursor(0, 0);
    nav.reset();
  }
  return proceed;
}

result actChangeScale(eventMask e,navNode& nav ,prompt& item) {
  if (e==exitEvent) {
    tft.setTextSize(Myconfig.scale);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("ESP32 Eprom programmer");
    tft.setCursor(0, 0);
    nav.reset();
  }
  return proceed;
}

result actListSettings(eventMask e, navNode& nav, prompt& item) {
  File f = FFat.open("/config.json","r");
  if (!f) {
    Serial.println();
    Serial.println("Nem találom a konfigurációs fájl!");
  } else {
    while (true) {
      String str1 = f.readString();
      if (str1 == NULL) break;
      Serial.println(str1);
    }
  }
  Serial.println();
  return proceed;
}

result actSplash(eventMask e, navNode& nav, prompt& item) {
  //Ez még csak teszt, később meg kell írni....
  fex.drawBmp(FFat,"/Betti2.bmp",0,0);
  delayMicroseconds(5000);
  nav.reset();
  return proceed;
}


unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(TFT_BLACK);
  tft.fillScreen(TFT_RED);
  tft.fillScreen(TFT_GREEN);
  tft.fillScreen(TFT_BLUE);
  tft.fillScreen(TFT_BLACK);
  return micros() - start;
}
/***************************************************************************************
** Function name:           testText
** Description:             Szöveg kiírás tesztelése
***************************************************************************************/
unsigned long testText() {
  tft.fillScreen(TFT_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(TFT_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  //tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(TFT_BLACK);

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing
  yield();
  tft.fillScreen(TFT_BLACK);

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;
  yield();
  tft.fillScreen(TFT_BLACK);

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;
  yield();
  tft.fillScreen(TFT_BLACK);

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  yield();
  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (y = 0; y < h; y += 5) tft.drawFastHLine(0, y, w, color1);
  for (x = 0; x < w; x += 5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(TFT_BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for (i = 2; i < n; i += 6) {
    i2 = i / 2;
    tft.drawRect(cx - i2, cy - i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  n = min(tft.width(), tft.height());
  for (i = n - 1; i > 0; i -= 6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx - i2, cy - i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx - i2, cy - i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (x = radius; x < w; x += r2) {
    for (y = radius; y < h; y += r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                      w = tft.width()  + radius,
                      h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for (x = 0; x < w; x += r2) {
    for (y = 0; y < h; y += r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  n     = min(cx, cy);
  start = micros();
  for (i = 0; i < n; i += 5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(0, 0, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (i = min(cx, cy); i > 10; i -= 5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(0, i, i));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(i, i, 0));
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for (i = 0; i < w; i += 6) {
    i2 = i / 2;
    tft.drawRoundRect(cx - i2, cy - i2, i, i, i / 8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

void testRotation() {
  tft.fillScreen(TFT_BLACK);
  for (uint8_t rotation = 0; rotation < 4; rotation++) {
    tft.setRotation(rotation);
    testText();
    delay(2000);
  }
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (i = min(tft.width(), tft.height()); i > 20; i -= 6) {
    i2 = i / 2;
    tft.fillRoundRect(cx - i2, cy - i2, i, i, i / 8, tft.color565(0, i, 0));
    yield();
  }

  return micros() - start;
}

result actScreenTest(eventMask e, navNode& nav, prompt& item) {
  unsigned long total = 0;
  unsigned long tn = 0;
  tn = micros();
  tft.fillScreen(TFT_BLACK);
  yield(); Serial.println(F("Teszt                    Idő  (microseconds)"));

  yield(); Serial.print(F("Screen fill              "));
  yield(); Serial.println(testFillScreen());

  yield(); Serial.print(F("Text                     "));
  yield(); Serial.println(testText());

  yield(); Serial.print(F("Lines                    "));
  yield(); Serial.println(testLines(TFT_CYAN));

  yield(); Serial.print(F("Horiz/Vert Lines         "));
  yield(); Serial.println(testFastLines(TFT_RED, TFT_BLUE));

  yield(); Serial.print(F("Rectangles (outline)     "));
  yield(); Serial.println(testRects(TFT_GREEN));

  yield(); Serial.print(F("Rectangles (filled)      "));
  yield(); Serial.println(testFilledRects(TFT_YELLOW, TFT_MAGENTA));

  yield(); Serial.print(F("Circles (filled)         "));
  yield(); Serial.println(testFilledCircles(10, TFT_MAGENTA));

  yield(); Serial.print(F("Circles (outline)        "));
  yield(); Serial.println(testCircles(10, TFT_WHITE));

  yield(); Serial.print(F("Triangles (outline)      "));
  yield(); Serial.println(testTriangles());

  yield(); Serial.print(F("Triangles (filled)       "));
  yield(); Serial.println(testFilledTriangles());

  yield(); Serial.print(F("Rounded rects (outline)  "));
  yield(); Serial.println(testRoundRects());

  yield(); Serial.print(F("Rounded rects (filled)   "));
  yield(); Serial.println(testFilledRoundRects());

  yield(); Serial.println(F("Done!")); yield();

  testRotation();
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  nav.reset();
  tft.setRotation(Myconfig.scrTajol);
  
  return proceed;
}
  // if (Myconfig.conType == 0) {
  //   int packetSize = udp.parsePacket();
  //   if (packetSize) {
  //     char packetBuffer[255];
  //     int len = udp.read(packetBuffer, 255);
  //     if (len > 0) {
  //       packetBuffer[len] = 0;
  //     }
  //     remoteIP = udp.remoteIP();
  //     remotePort = udp.remotePort();
  //     parseCommand(packetBuffer, remoteIP, remotePort);
  //   }
  // }

 void getFile(String fname,uint32_t fsize) {
  const char command[] = "READYFORRECEIVE";
  uint32_t readed = 0;
  File f = FFat.open(fname,"w");
  if (!f) {
    Serial.println("");
    Serial.println("Nem sikerült a fájl létrehozása!");
    return;
  }
  udp.write((const uint8_t *)command, strlen(command));
  // fájl fogadása
  while(readed < fsize) {
    int pckSize = udp.parsePacket();
    char pckBuff[255];
    int len = udp.read(pckBuff,255);
    if (len > 0) {
      f.write((uint8_t *)pckBuff,len);
      readed += len;
    }

  }
  f.flush();
  f.close();
 }