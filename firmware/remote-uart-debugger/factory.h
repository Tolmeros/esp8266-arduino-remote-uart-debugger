#include "user_settings.h"

//ESP_MODE
#define WIFI_AP_MODE    (0U)
#define WIFI_STA_MODE   (1U)

#if !defined(ESP_MODE)
  #define ESP_MODE              WIFI_STA_MODE
#endif

#if !defined(AP_NAME)
  #define AP_NAME               ("LedLamp")
#endif

// пароль WiFi точки доступа
#if !defined(AP_PASS)
  #define AP_PASS               ("31415926")
#endif

// статический IP точки доступа (лучше не менять)
#define AP_STATIC_IP_4        192
#define AP_STATIC_IP_3        168
#define AP_STATIC_IP_2        4
#define AP_STATIC_IP_1        1

#if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 0)
# undef(SERIAL_DEBUG)
#endif

#if defined(ESP8266_USE_WATCHDOG) && (ESP8266_USE_WATCHDOG == 0)
# undef(ESP8266_USE_WATCHDOG)
#endif
