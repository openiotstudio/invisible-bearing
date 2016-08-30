

#include <Wire.h>
#include <SFE_LSM9DS0.h>
#include <SPI.h>
#include <WiFi101.h>
#include <TelegramBot.h>
#include <ArduinoJson.h>
#include <ArduinoCloud.h>

// Arduino Cloud settings and credentials
const char userName[]   = "***";
const char thingName[] = "Master";
const char thingId[]   = "***";
const char thingPsw[]  = "***";


#define LSM9DS0_XM  0x1D // Would be 0x1E if SDO_XM is LOW
#define LSM9DS0_G   0x6B // Would be 0x6A if SDO_G is LOW
LSM9DS0 dof(MODE_I2C, LSM9DS0_G, LSM9DS0_XM);
#define PRINT_CALCULATED
#define PRINT_SPEED 500 // 500 ms between prints
#define threshold 30
#define REC A6

float heading;
unsigned long timer;
String line = "";
int best_dist = 360;
int i = 0;
int best_id;
int old_mapped_value=0;

const char* ssid = "***";
const char* pass = "***";


const char* host = "agent.electricimp.com";
const int httpsPort = 443;


WiFiSSLClient client;
ArduinoCloudThing Master;

void setup()
{

  Serial.begin(19200);
  
  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  Serial.println("");
  Serial.println("WiFi connected");


  uint16_t status = dof.begin();
  Serial.print("LSM9DS0 WHO_AM_I's returned: 0x");
  Serial.println(status, HEX);
  Serial.println("Should be 0x49D4");
  Serial.println();
  pinMode(REC, INPUT);

  Master.begin(thingName, userName, thingId, thingPsw, client);
  Master.enableDebug();
  // define the properties
  Master.addProperty("LedNumbers", INT, RW);
  Master.addProperty("flag", INT, RW);

  Serial.println("setted");
  httpRequest();

}

void loop()
{

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
  }

  
  int sensorValue = analogRead(REC);
  Serial.println(sensorValue);
  //  Master.writeProperty("flag", 0);

  if (sensorValue < 150) {

    Master.writeProperty("flag", 1);
    delay(100);
    dof.readGyro();
    dof.readAccel();
    dof.readMag();
    printHeading((float) dof.mx, (float) dof.my);
    Serial.println();
    int Best_Angle = compareAngles();
    

    if (Best_Angle <= 3) {
      Serial.print("Found One:  ");
      Serial.println(best_id);
      Master.writeProperty("LedNumbers", 8);
      httpPost();
    }

    else if (Best_Angle > 3) {
      int mappedValue = map(Best_Angle, 0, 180, 7, 0);
      if (mappedValue == old_mapped_value ) mappedValue=mappedValue-1;
      Serial.print("Mapped Value: ");
      Serial.println(mappedValue);
      old_mapped_value=mappedValue;
      Master.poll();
      Master.writeProperty("LedNumbers", mappedValue);
    }
    delay(6000);
  }
  delay(50);

  //  if ( millis() - timer > 900000) {
  //    line.remove(0);
  //    client.flush();
  //    client.stop();
  //    httpRequest();
  //    timer = millis();
  //
  //  }
}


void printHeading(float hx, float hy)     ////////////////////////////////////// PRINT HEADING //////////////////////////////////////////
{

  if (hy > 0) heading = 90 - (atan(hx / hy) * (180 / PI));
  else if (hy < 0) heading = 270 - (atan(hx / hy) * (180 / PI));
  else if ( hy == 0 && hx < 0) heading = 180;
  else if ( hy == 0 && hx > 0) heading = 0;

  Serial.print("Heading: ");
  Serial.println(heading, 0);
}


void httpRequest() {                        ////////////////////////////////////// HTTP GET //////////////////////////////////////////



  if (!client.connected()) {
    Serial.println("Reconnecting");
    client.connect(host, httpsPort);
    delay (2000);
  }

  String GetRequest = "GET //kTUnDA1vukGR/ships.json HTTP/1.1";

  client.println(GetRequest);
  client.println("Host: agent.electricimp.com");
  client.println("Cache-Control: no-cache");
  client.println("Postman-Token: 8f3808bc-502f-5099-8dcd-12420b5fe1d8");
  client.println("");

  Serial.println("request sent");


  while (client.connected()) {
    line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      line = "";
      for (int i = 0; i <= 30; i++) {
        line = line +  client.readStringUntil('}') + '}';
      }
      line = line + ']';
      break;
    }
  }
  Serial.println(line);
  client.flush();
  client.stop();
}


int compareAngles() {       ////////////////////////////////////// COMPARE ANGLES//////////////////////////////////////////

  DynamicJsonBuffer jsonBuffer;
  JsonArray& root = jsonBuffer.parseArray(line);
  if (!root.success()) {
    Serial.println("parseArray() failed");
  }

  best_dist = 360;

  for ( int i = 1; i < 30; i++) {
    int angle = root[i]["brg"];
    int dist = abs(heading - angle);
    Serial.println(angle);
    if (dist < best_dist) {
      best_dist = dist;
      best_id = root[i]["mmsi"];
      
    }
    
  }

  Serial.print("Best Dist: ");
  Serial.println(best_dist);
  return best_dist;
}

void httpPost() {                        ////////////////////////////////////// HTTP POST //////////////////////////////////////////

  client.flush();
  client.stop();


  if (!client.connected()) {
    Serial.println("Reconnecting");
    client.connect(host, httpsPort);
    delay (2000);
  }

  String data = "{\"mmsi\":\"";
  data = data + best_id;
  data = data + "\"}";
  Serial.println(data);

  client.println("POST /kTUnDA1vukGR/print HTTP/1.1");
  client.println("Host: agent.electricimp.com");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println();
  client.print(data);

  String line = client.readStringUntil('\r');
  Serial.println(line);

  client.stop();  // DISCONNECT FROM THE SERVER

  Serial.println("Post request sent");
  client.flush();
  client.stop();
}


