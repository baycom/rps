#ifndef _RPS_H
#define _RPS_H
#include <Arduino.h>
#include <LoRaLib.h>
#include <SSD1306Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include "cfg.h"
#include "lrs.h"
#include "pocsag.h"
#include "index_html.h"
#include "script_js.h"

//#define USE_QUEUE 

#define OLED_ADDRESS 0x3c
#define OLED_SDA 4  // GPIO4
#define OLED_SCL 15 // GPIO15
#define OLED_RST 16 // GPIO16

#define LoRa_RST 14  // GPIO 14
#define LoRa_CS 18   // GPIO 18
#define LoRa_DIO0 26 // GPIO 26
#define LoRa_DIO1 33 // GPIO 33
#define LoRa_DIO2 32 // GPIO 32

#define WIFI_ACCESSPOINT false
#define WIFI_STATION true
#define VERSION_STR "1.0"
#define cfg_ver_num 0x5
typedef struct {
  byte version;
  char wifi_ssid[33];
  char wifi_secret[65];
  char wifi_hostname[256];
  bool wifi_opmode;
  bool wifi_powersave;
  float tx_frequency;
  float tx_deviation;
  int8_t tx_power;
  uint8_t tx_current_limit;

  byte restaurant_id;
  byte system_id;
  byte alert_type;
  byte default_mode;
  int pocsag_baud;
} settings_t;

typedef struct {
  int restaurant_id; 
  int system_id; 
  int pager_number; 
  int alert_type;
} pager_t;

extern SX1278 fsk;
extern settings_t cfg;

#endif