/********************************************
 * WifiManager code with FS parameters (unused yet)
 * based on tzapu's filesystem code
 * https://github.com/tzapu/WiFiManager
 ********************************************/

#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "wifi.h"

#define CONFIGAP_SSID "PrayerClockAP"

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void resetWiFi() {
	Serial.println("Disconnecting WiFi");
	WiFi.persistent(false);      
	WiFi.disconnect(true);
	delay(2000);
	WiFi.persistent(true);
	Serial.println("Reset settings");
	WiFiManager wifiManager;
	wifiManager.resetSettings();
	delay(2000);
	Serial.println("Resetting ESP");
	ESP.eraseConfig();
	delay(2000);
	ESP.reset();
}

void setupWiFi() {
  // read configuration from FS json
  Serial.println("mounting FS...");
 // bool formatted = SPIFFS.format();

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    //SPIFFS.remove("/config.json");
    //SPIFFS.remove("/config1.json");
    //SPIFFS.remove("/config2.json");
    if (SPIFFS.exists("/config.json")) {
      // file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        
        StaticJsonDocument<1204> doc;
        DeserializationError err =  deserializeJson(doc,buf.get());
        if (!err) {
          Serial.println("\nparsed json");
          
          strcpy(ntpserver, doc["ntpserver"]);
          strcpy(apitoken, doc["apitoken"]);
          Serial.println(ntpserver);
          Serial.println(apitoken);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
		else {
      Serial.println("config.json does not exist");
		}
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
	WiFiManagerParameter custom_ntpserver("ntpserver", "NTP Server ", ntpserver, NTP_LEN);
	WiFiManagerParameter custom_apitoken("apitoken", "Lamad API token", apitoken, APITOKEN_LEN);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // set static ip
  // wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_ntpserver);
  wifiManager.addParameter(&custom_apitoken);

	WiFi.mode(WIFI_AP_STA);

  if (!wifiManager.autoConnect(CONFIGAP_SSID)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  // read updated parameters
  strcpy(ntpserver, custom_ntpserver.getValue());
  strcpy(apitoken, custom_apitoken.getValue());

  // save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    //DynamicJsonBuffer jsonBuffer;
    StaticJsonDocument<1204> doc;
    //JsonObject& json = jsonBuffer.createObject();
    doc["ntpserver"] = ntpserver;
    doc["apitoken"] = apitoken;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    serializeJson(doc,Serial);
    serializeJson(doc,configFile);
    //doc.printTo(Serial);
    //doc.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println("API token");
  Serial.println(apitoken);
  Serial.println("NTP Server");
  Serial.println(ntpserver);
}
