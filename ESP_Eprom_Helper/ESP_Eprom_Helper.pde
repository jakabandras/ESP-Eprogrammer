import uibooster.*;
import uibooster.components.*;
import uibooster.model.*;
import uibooster.model.formelements.*;
import uibooster.model.options.*;
import uibooster.utils.*;
import processing.net.*;
import java.util.zip.CRC32;

File file;
Form form;
UiBooster booster;
boolean selected = false;
PImage image;
boolean received = false;
Client client;
CRC32 crc;

void setup() {
  size(800,400);
  
  textAlign(CENTER, CENTER);
  client = new Client(this, "192.168.1.6", 1234);
  crc = new CRC32();
  
  booster = new UiBooster();
  booster.createTrayMenu("ESP Eprom Helper","clown.png")
    .withPopupMenu()
    .addMenu("Fájl küldése", ()->sendFile())
    .addMenu("Fájl fogadása",()->receiveFile())
    .addMenu("Kilépés",()->exit());
  
}

void sendBinaryFile(String filePath) {
  // Megnyitja a bináris fájlt és elküldi a tartalmát a szervernek
  File file = new File(filePath);
  if (file.exists()) {
    byte[] data = loadBytes(file);
    for (byte b : data) {
      client.write(b);
      if (client.available()>0) {
        String resp = client.readString();
        if (resp.compareTo("ERROR") == 0) {
          println("Hiba az átvitel során!");
          return;
        }
      }
      crc.update(b);
    }
    while (client.available() == 0) {};
    String msg = client.readString();
    if (msg.compareTo("TRANSFER_OK") == 0) {
      println("A fájl átvitele sikeres, CRC ellenőrzés");
      client.write(str(crc.getValue()));
      
    }
  } else {
    println("Nem találom a fájlt!");
  }
}

void sendFile() {
  file = booster.showFileSelection("*.jpg");
  image = loadImage(file.getAbsolutePath());
  image.resize(320,480);
  //image.save("d:\\ArduinoSoftwares\\ESP Eprogrammer\\DATA\\"+file.getName()+".bmp");
  println(file.getAbsolutePath());
  booster.showPicture("Küldendő kép",file).showImage();
  int fsize = (int)file.length();
  String xxx = "PUTFILE|"+file.getAbsolutePath()+"|"+String.valueOf(fsize);
  client.write(xxx);
  if (file.exists()) {
    String resp = "";
    while (resp.compareTo("WAITING_TO_FILE") != 0) {
      while (client.available() == 0) {};
      resp =  client.readString();
    };
    selected = true;
    sendBinaryFile(file.getAbsolutePath());
  }
}

void receiveFile()
{
}

void draw()
{
  background(10);
  while (true) {
    if (selected) {
    }
  }
  //loop();
}

void convertToBMP(String inputFileName, String outputFileName) {
  PImage img = loadImage(inputFileName);
  
  if (img == null) {
    println("A fájl nem található vagy nem megfelelő formátumú.");
    return;
  }
  
  int newWidth, newHeight;
  
  // Döntsd el az új méretet az arány megtartása alapján
  if (img.width > img.height) {
    newWidth = 320;
    newHeight = int(img.height * 320.0 / img.width);
  } else {
    newWidth = int(img.width * 480.0 / img.height);
    newHeight = 480;
  }
  
  // Átméretezés
  img.resize(newWidth, newHeight);
  
  // Készítsünk egy új képet az új mérettel
  PImage resizedImg = createImage(newWidth, newHeight, RGB);
  resizedImg.copy(img, 0, 0, img.width, img.height, 0, 0, newWidth, newHeight);
  
  // Mentsük el BMP formátumban
  resizedImg.save(outputFileName);
  
  println("A fájl sikeresen átkonvertálva és elmentve: " + outputFileName);
}


void convertTo16BitJPG(String inputFileName, String outputFileName) {
  PImage img = loadImage(inputFileName);
  
  if (img == null) {
    println("A fájl nem található vagy nem megfelelő formátumú.");
    return;
  }
  
  // Átméretezés
  img.resize(320, 480);
  
  // Létrehozunk egy új PImage-t a 16 bites árnyalatokkal
  PImage convertedImg = createImage(img.width, img.height, RGB);
  img.loadPixels();
  convertedImg.loadPixels();
  
  for (int i = 0; i < img.pixels.length; i++) {
    int c = img.pixels[i];
    
    // Az eredeti 24-bites RGB színből 16-bitesre konvertáljuk
    int r16 = c >> 11 & 0x1F;  // 5 bites piros
    int g16 = c >> 5 & 0x3F;   // 6 bites zöld
    int b16 = c & 0x1F;        // 5 bites kék
    
    // Az új 16-bites szín összeállítása
    int newColor = (r16 << 11) | (g16 << 5) | b16;
    
    convertedImg.pixels[i] = newColor;
  }
  
  convertedImg.updatePixels();
  
  // Mentsük el a fájlt
  convertedImg.save(outputFileName);
  
  println("A fájl sikeresen átkonvertálva és elmentve: " + outputFileName);
}

void receive( byte[] data, String ip, int port ) {  // <-- extended handler
  
  
  // get the "real" message =
  // forget the ";\n" at the end <-- !!! only for a communication with Pd !!!
  data = subset(data, 0, data.length-2);
  String message = new String( data );
  if (message == "DONE" ) received = true;
  // print the result
  println( "receive: \""+message+"\" from "+ip+" on port "+port );
}
