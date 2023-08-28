#include <Arduino.h>
#include <string>
#include <vector>
#include "pinout.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TaskScheduler.h>
#include <Adafruit_GFX.h>
#include <ILI9488.h>
#include <Wifi.h>
#include <WiFiUdp.h>


#include <ClickEncoder.h>


#include <menu.h>
#include <menuIO/adafruitGfxOut.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include <menuIO/chainStream.h>
//#include <plugin/SdFatMenu.h>
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
#define ILI9488_GRAY RGB565(128,128,128)
#define textScale 1
#define FORMAT_SPIFFS_IF_FAILED true

const colorDef<uint16_t> colors[6] MEMMODE={
  {{(uint16_t)ILI9488_BLACK,(uint16_t)ILI9488_BLACK}, {(uint16_t)ILI9488_BLACK, (uint16_t)ILI9488_BLUE,  (uint16_t)ILI9488_BLUE}},//bgColor
  {{(uint16_t)ILI9488_GRAY, (uint16_t)ILI9488_GRAY},  {(uint16_t)ILI9488_WHITE, (uint16_t)ILI9488_WHITE, (uint16_t)ILI9488_WHITE}},//fgColor
  {{(uint16_t)ILI9488_WHITE,(uint16_t)ILI9488_BLACK}, {(uint16_t)ILI9488_YELLOW,(uint16_t)ILI9488_YELLOW,(uint16_t)ILI9488_RED}},//valColor
  {{(uint16_t)ILI9488_WHITE,(uint16_t)ILI9488_BLACK}, {(uint16_t)ILI9488_WHITE, (uint16_t)ILI9488_YELLOW,(uint16_t)ILI9488_YELLOW}},//unitColor
  {{(uint16_t)ILI9488_WHITE,(uint16_t)ILI9488_GRAY},  {(uint16_t)ILI9488_BLACK, (uint16_t)ILI9488_BLUE,  (uint16_t)ILI9488_WHITE}},//cursorColor
  {{(uint16_t)ILI9488_WHITE,(uint16_t)ILI9488_YELLOW},{(uint16_t)ILI9488_BLUE,  (uint16_t)ILI9488_RED,   (uint16_t)ILI9488_RED}},//titleColor
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
result actRomRead(eventMask e, prompt &item);
result actRomWrite(eventMask e, prompt &item);
result actRomErase(eventMask e, prompt &item);
result actRomEraseFast(eventMask e, prompt &item);
result actRomCheck(eventMask e, prompt &item);
result actSelConnect(eventMask e, prompt &item);
result actSaveSettings(eventMask e, prompt &item);
result actLoadSettings(eventMask e, prompt &item);
result actListFiles(eventMask e, prompt &item);
result actHttpConnect(eventMask e, prompt &item);
//result filePick(eventMask event, navNode& nav, prompt &item);
void listSPIFFS();
void setDefConfig();
void timerIsr();

int sendAddress(uint16_t address);


// Globális változók
ILI9488 tft = ILI9488(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
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
ClickEncoder *encoder;
//ESP8266Timer ITimer;
Config Myconfig;
Task t1(1000, TASK_FOREVER, &timerIsr);
WiFiUDP udp;


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
  ,OP("Fájl kiválasztása",doNothing,enterEvent)
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
MENU(mnuConnections,"Csatlakozások",doNothing,noEvent,noStyle
  ,SUBMENU(mnuConType)
  ,EDIT("SSID :",Myconfig.ssid,alphaNumMask,doNothing,noEvent,noStyle)
  ,EDIT("Jelszó :",Myconfig.pass,alphaNumMask,doNothing,noEvent,noStyle)
  ,EDIT("IP cím :",Myconfig.ipaddr,alphaNumMask,doNothing,noEvent,noStyle)
  ,FIELD(Myconfig.udpport,"Port :","",0,65535,1,1,doNothing,noEvent,noStyle)
  ,EXIT("<Vissza")
);

SELECT(Myconfig.fileStorage,mnuStorage,"Romok tárolása :",doNothing,noEvent,noStyle
  ,VALUE("SPIFFS",0,doNothing,noEvent)
  ,VALUE("SD kártya",1,doNothing,noEvent)
);

MENU(mnuScreen,"Kijelző beállítása",doNothing,noEvent,noStyle
  ,OP("Kijelző teszt",doNothing,enterEvent)
  ,OP("Kijelző beállítása",doNothing,enterEvent)
  ,EXIT("<Vissza")
);

MENU(mnuMySettings,"Beállítások",doNothing,noEvent,noStyle
  ,SUBMENU(mnuConnections)
  ,SUBMENU(mnuStorage)
  ,SUBMENU(mnuScreen)
  ,OP("Beállítások mentése",doNothing,enterEvent)
  ,OP("Beállítások betöltése",doNothing,enterEvent)
  ,EXIT("<Vissza")
);

// TOGGLE(ledCtrl,setLed,"Led: ",doNothing,noEvent,noStyle//,doExit,enterEvent,noStyle
//   ,VALUE("On",HIGH,doNothing,noEvent)
//   ,VALUE("Off",LOW,doNothing,noEvent)
// );


// CHOOSE(chooseTest,chooseMenu,"Choose",doNothing,noEvent,noStyle
//   ,VALUE("First",1,doNothing,noEvent)
//   ,VALUE("Second",2,doNothing,noEvent)
//   ,VALUE("Third",3,doNothing,noEvent)
//   ,VALUE("Last",-1,doNothing,noEvent)
// );

// uint16_t year=2017;
// uint16_t month=10;
// uint16_t day=7;

//define a pad style menu (single line menu)
//here with a set of fields to enter a date in YYYY/MM/DD format
//altMENU(menu,birthDate,"Birth",doNothing,noEvent,noStyle,(systemStyles)(_asPad|Menu::_menuData|Menu::_canNav|_parentDraw)
// PADMENU(birthDate,"Birth",doNothing,noEvent,noStyle
//   ,FIELD(year,"","/",1900,3000,20,1,doNothing,noEvent,noStyle)
//   ,FIELD(month,"","/",1,12,1,0,doNothing,noEvent,wrapStyle)
//   ,FIELD(day,"","",1,31,1,0,doNothing,noEvent,wrapStyle)
// );

MENU(mainMenu,"Main menu",zZz,noEvent,wrapStyle
  ,SUBMENU(mnuEprom)
  ,SUBMENU(mnuFile)
  ,SUBMENU(mnuMySettings)
  ,EXIT("<Back")
);

MENU_OUTPUTS(out,MAX_DEPTH
  ,SERIAL_OUT(Serial)
  ,ADAGFX_OUT(tft,colors,6*textScale,9*textScale,{0,0,14,8},{14,0,14,8})
);

serialIn serial(Serial);
NAVROOT(nav,mainMenu,MAX_DEPTH,serial,out);


void setup() {
  Serial.begin(115200);
  while(!Serial);

  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS fájlrendszer csatolva");
  if (!SPIFFS.exists("/config.json")) {
    Serial.println("A konfigurációs fájl nem létezik, létrehozás...");
    File configFile = SPIFFS.open("/config.json", "w");
    setDefConfig();
    if (!configFile) {
      Serial.println("Nem sikerült megnyitni a konfigurációs fájlt írásra");
    }
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
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
      Serial.println("Nem sikerült megnyitni a konfigurációs fájlt olvasásra");
    }
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
      Serial.println("Nem sikerült beolvasni a konfigurációs fájlt");
      setDefConfig();
    } else {
      strcpy(Myconfig.ssid,doc["ssid"]);
      strcpy(Myconfig.pass,doc["pass"]);
      //strcpy(Myconfig.ipaddr,doc["ipaddr"]);
      Myconfig.udpport = doc["udpport"];
      Myconfig.conType = doc["conType"];
      Myconfig.fileStorage = doc["fileStorage"];
      Myconfig.romCheck = doc["romCheck"];
    }
  }
  Serial.println("Konfigurációs fájl beolvasva");
  if (Myconfig.conType == 0 || Myconfig.conType == 1) {
    WiFi.begin(Myconfig.ssid, Myconfig.pass);
    Serial.print("Csatlakozás a ");
    Serial.print(Myconfig.ssid);
    Serial.println(" hálózathoz");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
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
  listSPIFFS();
  encoder = new ClickEncoder(ENCODER_PINA, ENCODER_PINB, ENCODER_BTN, 4);
  delay(500);
  SPI.begin();
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9488_BLACK);
  tft.setTextColor(ILI9488_RED, ILI9488_BLACK);
  tft.setTextSize(textScale);
  tft.setCursor(0, 0);
  tft.println("ESP32 Eprom programmer");
  Serial.println("ESP32 Eprom programmer");Serial.flush();
  nav.timeOut=70;
  nav.idleTask=idle;//point a function to be used when menu is suspended
}

void loop() {
  nav.poll();
  digitalWrite(LEDPIN, ledCtrl);
  delay(100);//simulate a delay when other tasks are done
}

// Függvények és eljárások

result showEvent(eventMask e,navNode& nav,prompt& item) {
  Serial.print("event: ");
  Serial.println(e);
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
    File root = SPIFFS.open("/");
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
  File root = SPIFFS.open("/");
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

result actHttpConnect(eventMask e,prompt& item) {
  nav.idleOn(idleHttpConnect);
  return proceed;
}


// Interrupt rutin
void timerIsr() {
  encoder->service();
}
