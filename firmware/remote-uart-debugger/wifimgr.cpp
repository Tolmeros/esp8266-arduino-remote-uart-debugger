
#include "wifimgr.h"

WiFiMGR::WiFiMGR(JsonSettingsWiFi& wifi_settings, get_uptime_ms_func get_uptime_ms) {
	#if defined(WIFIMGR_DEBUG)
	DEBUG_WM(F("WiFiMGR init"));
	#endif

	this->wifi_settings = &wifi_settings;
	this->get_uptime_ms = get_uptime_ms;
	this->was_disconnected = false;

	WiFi.persistent(false);
	
	connection_lost_time = get_uptime_ms();

	handle();
}

void WiFiMGR::handle() {
	#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
	DEBUG_WM(F("WiFiMGR handle()"));
	#endif

	if (try_add_start) {
		#if defined(WIFIMGR_DEBUG)
		DEBUG_WM(F("WiFiMGR handle() try_add_start"));
		#endif
		disconnect_all();
		try_add_run = true;
		try_add_start = false;
		return;
	}

	if (try_add_run) {
		//#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
		//DEBUG_WM(F("WiFiMGR handle() try_add_run"));
		//#endif

		if (WiFi.status() != WL_CONNECTED) {
			//#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
			//DEBUG_WM(F("WiFiMGR handle() try_add_run not connected"));
			//#endif
			//try_add_time
			if ((get_uptime_ms() - try_add_time >= WIFIMRG_TRY_CINNECT_TIMEOUT)) {
				#if defined(WIFIMGR_DEBUG)
				DEBUG_WM(F("WiFiMGR handle() try_add_run not connected timeout"));
				#endif
				try_add_tries--;
				if (try_add_tries < 1) {
					try_add_run = false;
				} else {
					try_add_time = get_uptime_ms();
					connect_to_SSID(try_add_ssid, try_add_password);
				}
			}
			return;

		} else {
			// add network or change
			int8_t id = wifi_settings->getNetworkIdBySsid(try_add_ssid);
			if (id >=0) {
				#if defined(WIFIMGR_DEBUG)
				DEBUG_WM(F("WiFiMGR handle() try_add_run chaged"));
				#endif
				wifi_settings->changeNetworkPassword(id, try_add_password);
				wifi_settings->changeNetworkHidden(id, try_add_hidden);
			} else {
				#if defined(WIFIMGR_DEBUG)
				DEBUG_WM(F("WiFiMGR handle() try_add_run added"));
				#endif
				wifi_settings->addNetwork(
					try_add_ssid,
					try_add_password,
					try_add_hidden
				);
			}
			try_add_run = false;
		}
		
	}


	// если в настройках установлен режим AP
	// или если не заданы сети WiFi
	if ((wifi_settings->getEspWiFiMode() == WIFI_AP_MODE) ||
		(wifi_settings->getStoredNetworksCount() == 0)) {
		#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
		DEBUG_WM(F("WiFiMGR handle() WIFI_AP_MODE or no networks"));
		#endif

		// если всё ещё не в режиме AP
		if (WiFi.getMode() != WIFI_AP) {
			// swtich to AP mode
			switch_to_AP();
		}
		return;
	} else if (wifi_settings->getEspWiFiMode() == WIFI_STA_MODE) {
		#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
		DEBUG_WM(F("WiFiMGR handle() WIFI_STA_MODE."));
		#endif

		if (WiFi.status() != WL_CONNECTED) {
			if ( connection_process && (get_uptime_ms() - try_connect_time < WIFIMRG_TRY_CINNECT_TIMEOUT)) {
				// still in connection process to network
				#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
				DEBUG_WM(F("WiFiMGR handle() WIFI_STA_MODE still in connection process to network."));
				#endif

				return;
			}

			#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
			DEBUG_WM(F("WiFiMGR handle() WIFI_STA_MODE not connected."));
			#endif

			if (!was_disconnected) {
				#if defined(WIFIMGR_DEBUG)
				// && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
				DEBUG_WM(F("WiFiMGR handle() was_disconnected"));
				#endif
				// waiting timeout
				connection_lost_time = get_uptime_ms();
				was_disconnected = true;
				// try another wifi
				if (first_run) {
					#if defined(WIFIMGR_DEBUG)
					DEBUG_WM(F("WiFiMGR handle() first_run"));
					#endif
					switch_station();
					first_run = false;
				}

				disconnected_state = TRY_AP;
			}
			else if (((get_uptime_ms() - connection_lost_time >= WIFIMRG_RECONNECTION_TIMEOUT) &&
						(disconnected_state == TRY_STA)) ||
					(disconnected_state == TRY_STA_IMMEDIATELY))
			{
				// try reconnect to wifi network
				switch_station();
				disconnected_state = TRY_AP;
				connection_lost_time = get_uptime_ms();
			}
			else if ((get_uptime_ms() - connection_lost_time >= WIFIMRG_CONNECTION_TIMEOUT) &&
					(disconnected_state == TRY_AP))
			{
				// swtich to AP mode
				switch_to_AP();
				disconnected_state = TRY_STA;
				connection_lost_time = get_uptime_ms();
			}
			else {
				// try another wifi
				//switch_station();

				//nop
			}
		} else {
			#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
			DEBUG_WM(F("WiFiMGR handle() WIFI_STA_MODE connected."));
			#endif
			was_disconnected = false;
			#if defined(WIFIMGR_DEBUG) && !defined(WIFIMGR_DEBUG_VERBOSE_HANDLE)
			if (connection_process) {
				DEBUG_WM(F("WiFiMGR handle() WIFI_STA_MODE connected."));
				DEBUG_WM(WiFi.localIP());
			}
			#endif
			connection_process = false;
			return;
		}
	}
	// опционально:
	// по таймоуту если в AP режиме - отключаться и сканить сети
	// по большему таймоуту в режиме STA - отключаться и сканить сети
	// не делать такого если OTA и ещё что-то критичное
}

void WiFiMGR::switch_to_AP() {
	IPAddress	_ap_static_ip;
	IPAddress	_ap_static_sn;

	const char*   _apName = AP_NAME;
	const char*   _apPassword = AP_PASS;

	#if defined(WIFIMGR_DEBUG) && defined(WIFIMGR_DEBUG_VERBOSE_SWITCH_TO_AP)
	DEBUG_WM(F("WiFiMGR switch_to_AP()"));
	DEBUG_WM(WiFi.getMode());
	DEBUG_WM(WIFI_AP);
	#endif

	if (WiFi.getMode() != WIFI_AP) {
		#if defined(WIFIMGR_DEBUG) && !defined(WIFIMGR_DEBUG_VERBOSE_SWITCH_TO_AP)
		DEBUG_WM(F("WiFiMGR switch_to_AP()"));
		#endif

		_ap_static_ip = IPAddress(AP_STATIC_IP_4,
								  AP_STATIC_IP_3,
								  AP_STATIC_IP_2,
								  AP_STATIC_IP_1
		);

		_ap_static_sn = IPAddress(255, 255, 255, 0);

		disconnect_all();

		WiFi.mode(WIFI_AP);
		WiFi.softAPConfig(_ap_static_ip, _ap_static_ip, _ap_static_sn);

		if (_apPassword != NULL) {
			WiFi.softAP(_apName, _apPassword);
		} else {
			WiFi.softAP(_apName);
		}
	}
	connection_process = false;
}

void WiFiMGR::connect_to_SSID(const char* ssid, const char* password) {
	#if defined(WIFIMGR_DEBUG_WAIT_AFTER_WIFI_BEGIN)
	int counter = 0;
	#endif

	if (WiFi.getMode() == WIFI_AP) {
		WiFi.softAPdisconnect();
	}
	WiFi.mode(WIFI_STA);

	#if defined(WIFIMGR_DEBUG)
	DEBUG_WM(F("WiFiMGR connect_to_SSID() WiFi.begin()"));
	#endif

	WiFi.begin(ssid, password);
	try_connect_time = get_uptime_ms();
	connection_process = true;

	#if defined(WIFIMGR_DEBUG_WAIT_AFTER_WIFI_BEGIN)
	#if defined(WIFIMGR_DEBUG)
	DEBUG_WM(F("WiFiMGR connect_to_SSID() WiFi.begin() wait"));
	#endif
	while ((WiFi.status() != WL_CONNECTED) && (counter < 256)) {
		#if defined(WIFIMGR_DEBUG)
		DEBUG_WM(F("."));
		#endif
		delay(500);
	}
	#if defined(WIFIMGR_DEBUG)
	DEBUG_WM(F("WiFiMGR connect_to_SSID() WiFi.begin() wait end"));
	#endif
	#endif
}

void WiFiMGR::switch_station() {
	listWiFiNetworks wifi_list[WIFI_CONFIG_NETWORKS_AMOUNT];
	int counter;
	String foundSSID;
	byte wifi_list_count = 0;
	int8_t wifi_id;
	char buf[WIFI_CONFIG_SSID_MAX_LENGTH];
	//bool found_priority_network = false;
	int8_t prio_wifi_id;

	#if defined(WIFIMGR_DEBUG)
	DEBUG_WM(F("WiFiMGR switch_station()"));
	#endif

	disconnect_all();

	// если список пуст - выйти
	if (wifi_settings->getStoredNetworksCount() == 0) {
		#if defined(WIFIMGR_DEBUG)
		DEBUG_WM(F("WiFiMGR switch_station() networks list empty"));
		#endif
		return;
	}

	// scan
	int foundNetworkCount = WiFi.scanNetworks();
	if(foundNetworkCount <= 0) {
		#if defined(WIFIMGR_DEBUG)
		DEBUG_WM(F("WiFiMGR switch_station() scan no networks"));
		#endif
		return;
	}

	#if defined(WIFIMGR_DEBUG)
	DEBUG_WM("found networks count: " + String(foundNetworkCount));
	#endif
	// просеить список найденых сетей через настройки
	// отсортировать
	int bestRSSI = -1000;
	int bestNetworkIndex = -1;

	counter = foundNetworkCount;
	#if defined(WIFIMGR_DEBUG)
	DEBUG_WM(F("WiFiMGR switch_station() scan networks sort:"));
	#endif

	prio_wifi_id = wifi_settings->getPriorityNetworkId();

	while (counter-- > 0) {
		foundSSID = WiFi.SSID(counter);
		//wifi_settings->networkExsists(foundSSID);
		foundSSID.toCharArray(buf, WIFI_CONFIG_SSID_MAX_LENGTH);
		wifi_id = wifi_settings->getNetworkIdBySsid(buf);
		//if (wifi_settings->networkExsists(foundSSID)) {
		if (wifi_id >= 0) {
			#if defined(WIFIMGR_DEBUG)
			DEBUG_WM(F("WiFiMGR switch_station() network in settings list"));
			DEBUG_WM(buf);
			#endif

			if (wifi_id == prio_wifi_id) {
				#if defined(WIFIMGR_DEBUG)
				DEBUG_WM(F("WiFiMGR switch_station() priority network found"));
				DEBUG_WM(buf);
				#endif
				connect_to_SSID(buf,
								wifi_settings->getNetworkPasswordById(wifi_id)
				);
				return;
			}

			//foundSSID.toCharArray(wifi_list[wifi_list_count].ssid, WIFI_CONFIG_SSID_MAX_LENGTH);
			strlcpy(wifi_list[wifi_list_count].ssid,
					buf,
					sizeof(wifi_list[wifi_list_count].ssid)
			);

			wifi_list[wifi_list_count].rssi = WiFi.RSSI(counter);
			wifi_list[wifi_list_count].id = wifi_id;
			// тут нужен оптимизейшен!
			// не нужен массив сетей, одной записи bestNetwrok будет достаточно
			

			if(bestRSSI < wifi_list[wifi_list_count].rssi) {
				bestRSSI = wifi_list[wifi_list_count].rssi;
				bestNetworkIndex = wifi_list_count;
			}

			wifi_list_count++;
		} // if (wifi_id >= 0)
	} // while (counter-- > 0)

	if (wifi_list_count>0) {
		#if defined(WIFIMGR_DEBUG)
		DEBUG_WM(F("WiFiMGR switch_station() filtered list not empty"));
		DEBUG_WM(wifi_list_count);
		#endif
		// есть ли в списке приоритетная сеть?

		// если нет - то к лучшей сети
		if (bestNetworkIndex >= 0) {
			connect_to_SSID(wifi_list[bestNetworkIndex].ssid,
							wifi_settings->getNetworkPasswordById(wifi_list[bestNetworkIndex].id)
			);
		}
	}

	
	// переключиться, если есть к чему
}

void WiFiMGR::disconnect_all() {
	#warning Let make it inline/static? 
	WiFi.softAPdisconnect();
	WiFi.disconnect();
}

bool WiFiMGR::try_add(const char* ssid, const char* password, const bool hidden) {

	if ((wifi_settings->getNetworkIdBySsid(try_add_ssid) < 0) &&
		(wifi_settings->getStoredNetworksCount() >= WIFI_CONFIG_NETWORKS_AMOUNT))
	{
		return false;
	}

	strlcpy(try_add_ssid,
			ssid,
			sizeof(try_add_ssid)
	);
	strlcpy(try_add_password,
			password,
			sizeof(try_add_password)
	);
	try_add_hidden = hidden;
	try_add_start = true;
	try_add_run = false;
	try_add_tries = 3;
	if (get_uptime_ms() - WIFIMRG_TRY_CINNECT_TIMEOUT > 0) {
		try_add_time = get_uptime_ms() - WIFIMRG_TRY_CINNECT_TIMEOUT;
	}

	return true;
}

#if defined(WIFIMGR_DEBUG)
template <typename Generic>
void WiFiMGR::DEBUG_WM(Generic text) {
  //if (_debug) {
    Serial.print(F("*WiFiMGR: "));
    Serial.println(text);
  //}
}
#endif
