
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <OneWire.h>              //DSB18B20 Temperature Sensor
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266HTTPClient.h>


int DS18S20_Pin = 0;
OneWire ds(DS18S20_Pin);  // on digital pin 2


float getTemp(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if (!ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if (OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (TemperatureSum > 2000) {
    return  TemperatureSum - 4096;
  }
  
  return TemperatureSum;
  
}

void setup() {
  Serial.begin(115200);
  WiFiManager wifiManager;

  if(!wifiManager.autoConnect("GelaCerva")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  Serial.print("Connecting to ");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

bool send_temp(float t){
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin("http://192.168.1.8:8000/api/temp/");  //Specify request destination
    http.addHeader("Content-Type", "application/json");
    String json_post = "{\"temperature\":" +  String(t) + ",\"device\":1}";
    int httpCode = http.POST(json_post); //Send the request 

    if (httpCode > 0) { //Check the returning code
      String payload = http.getString();   //Get the request response payload
      Serial.println(payload);
    }
    
    http.end();   //Close connection
    return true;
}

// the loop function runs over and over again forever
void loop() {
  float temperature = getTemp();
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
      send_temp(temperature);
  }
  delay(3000);
}
