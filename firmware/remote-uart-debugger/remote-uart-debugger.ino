// https://arduino.esp8266.com/stable/package_esp8266com_index.json

#if defined(ESP8266)
# include <ESP8266WiFi.h>
//# include <ESP8266WebServer.h>
# include <FS.h>
#elif defined(ESP32)
# include <WiFi.h>
//# include <WebServer.h>
# include <SPIFFS.h>
#endif

#include "factory.h"
#include "jsonsettings.h"
#include "wifimgr.h"

JsonSettingsWiFi settingsWiFi;

uint64_t get_uptime_ms() {
  /* http://arduino.palvelum.me/?p=177 */
  static uint32_t low32, high32;
  uint32_t new_low32 = millis();
  if (new_low32 < low32) high32++;
  low32 = new_low32;
  return (uint64_t) high32 << 32 | low32;
}

WiFiMGR wifiMgr(settingsWiFi, get_uptime_ms);

void setup() {
	#if defined(SERIAL_DEBUG)
	Serial.begin(115200);
	Serial.println();
	#endif

	#if defined(ESP8266) && defined(ESP8266_USE_WATCHDOG)
	ESP.wdtEnable(WDTO_8S);
	#endif

	#if defined(ESP8266)
  # if defined(SERIAL_DEBUG)
	if (!SPIFFS.begin()) {
		Serial.println(F("SPIFFS not mounted."));
	} else {
		Serial.println(F("SPIFFS mounted"));
	}
  # else
  SPIFFS.begin();
  # endif //defined(SERIAL_DEBUG)
	#elif defined(ESP32)
  # if defined(SERIAL_DEBUG)
	if (!SPIFFS.begin()) {
		Serial.println(F("SPIFFS not mounted. Try to format"));
		if (!SPIFFS.format()) {
			Serial.println(F("SPIFFS format succesful"));
		}
	} else {
		Serial.println(F("SPIFFS mounted"));
	}
  # else
  if (!SPIFFS.begin()) {
    SPIFFS.format();
  }
  # endif //defined(SERIAL_DEBUG)
  #endif
}

void loop() {
	#if defined(ESP8266) && defined(ESP8266_USE_WATCHDOG)
    ESP.wdtFeed();
  	#endif

}
