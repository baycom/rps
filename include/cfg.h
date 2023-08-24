#ifndef CFG_H
#define CFG_H

#define EEPROM_SIZE 4096
#define WIFI_ACCESSPOINT false
#define WIFI_STATION true
#define cfg_ver_num 0x8

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
//Version 6
  char ota_path[256];
//Version 7
  bool wifi_ap_fallback;
//Version 8
  char ip_addr[16];
  char ip_gw[16];
  char ip_netmask[16];
  char ip_dns[16];
} settings_t;

void write_config(void);
void read_config(void);
String get_settings(void);
boolean parse_settings(DynamicJsonDocument json);

#endif