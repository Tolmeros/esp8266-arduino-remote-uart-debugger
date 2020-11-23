#ifndef WIFIMGR_H
#define WIFIMGR_H

#if defined(ESP8266)
	#include <ESP8266WiFi.h>
#elif defined(ESP32)
	#include <WiFi.h>
#endif

//#include <DNSServer.h>

#include "factory.h"
#include "jsonsettings.h"

#define WIFIMGR_DEBUG 1
//#define WIFIMGR_DEBUG_VERBOSE_HANDLE 1
//#define WIFIMGR_DEBUG_VERBOSE_SWITCH_TO_AP 1
//#define WIFIMGR_DEBUG_WAIT_AFTER_WIFI_BEGIN 1

typedef uint64_t (*get_uptime_ms_func)(void);

struct listWiFiNetworks {
	Ssid ssid;
	int rssi;
	int id;
};

#define WIFIMRG_CONNECTION_TIMEOUT 10*1000
#define WIFIMRG_RECONNECTION_TIMEOUT 30*1000
#define WIFIMRG_TRY_CINNECT_TIMEOUT 15*1000

enum DisconnectedState {
	TRY_AP,
	TRY_STA,
	TRY_STA_IMMEDIATELY
};

class WiFiMGR {
private:
	JsonSettingsWiFi *wifi_settings;
	get_uptime_ms_func get_uptime_ms;
	uint64_t connection_lost_time = 0;
	uint64_t try_connect_time = 0;
	bool was_disconnected;
	byte disconnected_state = TRY_STA_IMMEDIATELY;
	bool connection_process = false;
	bool first_run = true;
	bool try_add_start = false;
	bool try_add_run = false;
	char try_add_ssid[WIFI_CONFIG_SSID_MAX_LENGTH];
	char try_add_password[WIFI_CONFIG_PASS_MAX_LENGTH];
	bool try_add_hidden;
	byte try_add_tries;
	uint64_t try_add_time = 0;

	#if defined(WIFIMGR_DEBUG)
	bool _debug = true;
	#endif

	/*
	IPAddress     _ap_static_ip;
    IPAddress     _ap_static_gw;
    IPAddress     _ap_static_sn;
    */

	void switch_to_AP();
	void connect_to_SSID(const char* ssid, const char* password);
	void switch_station();
	void disconnect_all();

	#if defined(WIFIMGR_DEBUG)
	template <typename Generic>
    void          DEBUG_WM(Generic text);
    #endif

public:
	WiFiMGR(JsonSettingsWiFi& wifi_settings, get_uptime_ms_func get_uptime_ms);

	void handle();
	bool try_add(const char* ssid, const char* password, const bool hidden=false);
	//void switch_to_AP();
	//void switch_station();
};

#endif
