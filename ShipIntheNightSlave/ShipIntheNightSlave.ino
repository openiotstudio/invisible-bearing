

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include <WiFi101.h>
#include <ArduinoCloud.h>

const char* ssid = "YourNetwork";
const char* pass = "YourPassword";


const char userName[]   = "***";
const char thingName[] = "Slave";
const char thingId[]   = "***";
const char thingPsw[]  = "***";

unsigned long timer;

WiFiSSLClient sslClient;
ArduinoCloudThing Slave;

int OldValue = 0;

#define PIN            6

#define NUMPIXELS      8
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGBW + NEO_KHZ800);

int delayval = 500;

void setup() {
  Serial.begin (9600);
  
  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  Serial.println("");
  Serial.println("WiFi connected");


  // setup the "cloudObject"
  Slave.enableDebug(); // eneble the serial debug output
  Slave.begin(thingName, userName, thingId, thingPsw, sslClient);

  // define the properties
  Slave.addExternalProperty("Master", "LedNumbers", INT); // this property is owned by "lampSwitch" object
  Slave.addProperty("LEDREAD", INT, RW);

  pixels.begin(); // This initializes the NeoPixel library.
}

void loop() {
  Slave.poll();
  

  int PixelValue = Slave.readProperty("Master", "LedNumbers").toInt();
  Serial.println(PixelValue);
  
  if (PixelValue != OldValue ) { 
    Slave.writeProperty("LEDREAD", PixelValue);

    for (int i = 0; i < PixelValue; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
      OldValue = PixelValue;
      delay(50);
    }
    delay(4000);
    for (int i = PixelValue; i >=0 ; i--) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
      delay(50);
    }
  }



 

}

