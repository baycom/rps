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
#include <EOTAUpdate.h>
#include "version.h"
#include "update.h"
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

typedef struct {
  int restaurant_id; 
  int system_id; 
  int pager_number; 
  int alert_type;
} pager_t;

extern SX1278 fsk;
extern settings_t cfg;

#endif