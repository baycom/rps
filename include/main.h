#ifndef _RPS_H
#define _RPS_H
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <RadioLib.h>
#include <SSD1306Wire.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <EOTAUpdate.h>

/*
 * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
 * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
 * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
 * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
 */
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

#ifdef LILYGO_POE
// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN 16
#else
#define ETH_POWER_PIN -1
#endif

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR 0

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN 23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN 18
#define NRST        5

#include <ETH.h>

#include "version.h"
#include "util.h"
#include "webserver.h"
#include "cfg.h"
#include "lrs.h"
#include "pocsag.h"
#include "retekess_ook_t112.h"
#include "retekess_ook_td161.h"
#include "retekess_fsk_td164.h"

//#define USE_QUEUE 
//#define DEBUG
#ifdef HELTEC

#define OLED_ADDRESS 0x3c
#define OLED_SDA 4  // GPIO4
#define OLED_SCL 15 // GPIO15
#define OLED_RST 16 // GPIO16

#define LoRa_RST 14  // GPIO 14
#define LoRa_CS 18   // GPIO 18
#define LoRa_DIO0 26 // GPIO 26
#define LoRa_DIO1 33 // GPIO 33 (Heltec v2: GPIO 35)
#define LoRa_DIO2 32 // GPIO 32 (Heltec v2: GPIO 34 has to be conected to GPIO 32)
#endif

#ifdef OLIMEX_POE
    #define LoRa_SCK  14 // (HS2_CLK)  
    #define LoRa_MOSO  2 // (HS2_DATA)
    #define LoRa_MISO 15 // (HS2_CMD)
    #define LoRa_CS    4 // (GPIO4)
    #define LoRa_RST   5 // (GPIO5)

    #define LoRa_DIO0 36 // (GPI36)
    #define LoRa_DIO1 13 // (GPIO13)
    #define LoRa_DIO2 16 // (GPIO16) 
#endif

#ifdef DEBUG
  #define dbg(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define err(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define info(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define warn(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
#else
  #define dbg(format, arg...) do {} while (0)
  #define err(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define info(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define warn(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
#endif 

typedef struct {
  int restaurant_id; 
  int system_id; 
  int pager_number; 
  int alert_type;
} pager_t;

extern hw_timer_t *timer;
extern SX1276 fsk;
extern settings_t cfg;
extern const uint8_t data_index_html_start[] asm("_binary_data_index_html_start");
extern const uint8_t data_index_html_end[] asm("_binary_data_index_html_end");
extern const uint8_t data_script_js_start[] asm("_binary_data_script_js_start");
extern const uint8_t data_script_js_end[] asm("_binary_data_script_js_end");

int call_pager(byte mode, int tx_power, float tx_frequency, float tx_deviation, int pocsag_baud, int restaurant_id, int system_id, int pager_number, int alert_type, bool reprogram_pager, func_t pocsag_telegram_type, const char *message, bool clear=false);

#endif