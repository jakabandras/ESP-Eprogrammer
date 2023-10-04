import uibooster.*;
import uibooster.components.*;
import uibooster.model.*;
import uibooster.model.formelements.*;
import uibooster.model.options.*;
import uibooster.utils.*;

import hypermedia.net.*;

UDP udp;
File file;
Form form;
UiBooster booster;

void setup() {
  size(800,400);
  udp = new UDP(this,1234,"192.168.1.8");
  
  
  textAlign(CENTER, CENTER);
  
  booster = new UiBooster();
  booster.createTrayMenu("ESP Eprom Helper","clown.png")
    .withPopupMenu()
    .addMenu("Fájl küldése", ()->sendFile())
    .addMenu("Fájl fogadása",()->receiveFile());
  
}

void sendFile() {
  file = booster.showFileSelection("*.jpg");
}

void receiveFile()
{
}

void draw()
{
  background(10);
}
