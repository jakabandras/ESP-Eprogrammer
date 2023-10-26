import uibooster.*;
import uibooster.components.*;
import uibooster.model.*;
import uibooster.model.formelements.*;
import uibooster.model.options.*;
import uibooster.utils.*;
import com.bckmnn.udp.*;
import java.net.InetSocketAddress;
import java.net.InetAddress;
import java.net.SocketAddress;
import hypermedia.net.*;

UDP udp;
File file;
Form form;
UiBooster booster;
boolean selected = false;
PImage image;
boolean received = false;

void setup() {
  size(800,400);
  udp = new UDP(this,666);
  udp.log(true);
  udp.loopback(true);
  udp.listen(true);
  
  textAlign(CENTER, CENTER);
  
  booster = new UiBooster();
  booster.createTrayMenu("ESP Eprom Helper","clown.png")
    .withPopupMenu()
    .addMenu("Fájl küldése", ()->sendFile())
    .addMenu("Fájl fogadása",()->receiveFile())
    .addMenu("Kilépés",()->exit());
  
}
void sendFileInChunks(String fn) {
  byte[] buffer = loadBytes(fn);
  int start = 0;
  while (start < buffer.length) {
    int end = min(start+512,buffer.length-start);
    byte[] chunk = subset(buffer,start,end);
    udp.send(chunk,"192.168.1.6",1234);
    while(!received) {};
    start += 512;
    
  }
}

void sendFile() {
  file = booster.showFileSelection("*.jpg");
  image = loadImage(file.getAbsolutePath());
  image.resize(320,480);
  image.save("d:\\ArduinoSoftwares\\ESP Eprogrammer\\DATA\\"+file.getName()+".bmp");
  println(file.getAbsolutePath());
  booster.showPicture("Küldendő kép",file).showImage();
  int fsize = (int)file.length();
  String xxx = "PUTFILE|"+file.getAbsolutePath()+"|"+String.valueOf(fsize);
  udp.send(xxx,"192.168.1.6",1234);
  
  if (file.exists()) {
    selected = true;
    sendFileInChunks(file.getAbsolutePath());
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
