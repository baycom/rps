#include "main.h"

void write_config(void)
{
  EEPROM.writeBytes(0, &cfg, sizeof(cfg));
  EEPROM.commit();
}

void read_config(void)
{
  EEPROM.readBytes(0, &cfg, sizeof(cfg));
  if (cfg.version != cfg_ver_num) {
    if (cfg.version == 0xff) {  
      uint8_t mac[10];
      WiFi.macAddress(mac);
      sprintf(cfg.wifi_ssid, "RPS-%02X%02X%02X", mac[3], mac[4], mac[5]);
      strcpy(cfg.wifi_secret, "");
      strcpy(cfg.wifi_hostname, cfg.wifi_ssid);
      cfg.wifi_opmode = OPMODE_WIFI_ACCESSPOINT;
      cfg.wifi_powersave = false;
      cfg.restaurant_id = 0x0;
      cfg.system_id = 0x0;
      cfg.alert_type = 0x1;
      cfg.default_mode = 0x0;
      cfg.pocsag_baud = 1200;
      cfg.tx_current_limit = 240;
      cfg.tx_power = 17;
      cfg.tx_deviation = 3.5;
      cfg.tx_frequency = 446.146973;
    }
    if(cfg.version == 0xff || cfg.version < 9) {
      cfg.pocsag_tx_deviation = 4.5;
      cfg.pocsag_tx_frequency = 446.146973;
      cfg.retekess_tx_frequency = 433.778;
      cfg.retekess_system_id = 200;
      cfg.multi_pager_types = 0;
    }
    if(cfg.version == 0xff || cfg.version < 10) {
      cfg.retekess_tx_deviation = 35;
    }
    if(cfg.version == 0xff || cfg.version < 11) {
      cfg.retekess_alert_type = 0;
    }
    if(cfg.version == 0xff || cfg.version < 12) {
      cfg.display_timeout = 5000;
    }
    if(cfg.version == 0xff || cfg.ota_path[0] == 0xff) {
            cfg.ota_path[0] = 0;
    }
  
    if(cfg.ip_addr[0] == 0xff || cfg.ip_gw[0] == 0xff || cfg.ip_netmask[0] == 0xff || cfg.ip_dns[0] == 0xff) {
      cfg.ip_addr[0] = 0;
      cfg.ip_gw[0] = 0;
      cfg.ip_netmask[0] = 0;
      cfg.ip_dns[0] = 0;
    }
  
    cfg.version = cfg_ver_num;
    write_config();
  }
  info("Settings:\n");
  info("cfg version     : %d\n", cfg.version);
  info("display_timeout : %ld\n", cfg.display_timeout);
  info("ssid            : %s\n", cfg.wifi_ssid);
  info("wifi_secret     : %s\n", cfg.wifi_secret);
  info("wifi_hostname   : %s\n", cfg.wifi_hostname);
  info("wifi_opmode     : %d\n", cfg.wifi_opmode);
  info("wifi_powersave  : %d\n", cfg.wifi_powersave);
  info("wifi_ap_fallback: %d\n", cfg.wifi_ap_fallback);
  info("ip_addr         : %s\n", cfg.ip_addr);
  info("ip_gw           : %s\n", cfg.ip_gw);
  info("ip_netmask      : %s\n", cfg.ip_netmask);
  info("ip_dns          : %s\n", cfg.ip_dns);
  info("ota_path        : %s\n", cfg.ota_path);
  info("restaurant_id   : %d\n", cfg.restaurant_id);
  info("system_id       : %d\n", cfg.system_id);
  info("alert_type      : %d\n", cfg.alert_type);
  info("default_mode    : %d\n", cfg.default_mode);
  info("pocsag_baud     : %d\n", cfg.pocsag_baud);
  info("tx_power        : %ddBm\n", cfg.tx_power);
  info("tx_current_limit: %dmA\n", cfg.tx_current_limit);
  info("lr_tx_frequency      : %.6fMHz\n", cfg.tx_frequency);
  info("lr_tx_deviation      : %.1fkHz\n", cfg.tx_deviation);
  info("pocsag_tx_frequency  : %.6fMHz\n", cfg.pocsag_tx_frequency);
  info("pocsag_tx_deviation  : %.1fkHz\n", cfg.pocsag_tx_deviation);
  info("retekess_tx_frequency: %.6fMHz\n", cfg.retekess_tx_frequency);
  info("retekess_tx_deviation: %.1fkHz\n", cfg.retekess_tx_deviation);
  info("retekess_system_id   : %d\n", cfg.retekess_system_id);
  info("retekess_alert_type  : %d\n", cfg.retekess_alert_type);
  info("multi_pager_types    : %d\n", cfg.multi_pager_types);
}

String get_settings(void)
{
  DynamicJsonDocument json(1024);
  json["version"] = VERSION_STR;
  json["alert_type"] = cfg.alert_type;
  json["wifi_hostname"] = cfg.wifi_hostname;
  json["restaurant_id"] = cfg.restaurant_id;
  json["system_id"] = cfg.system_id;
  json["wifi_ssid"] = cfg.wifi_ssid;
  json["wifi_opmode"] = cfg.wifi_opmode;
  json["wifi_ap_fallback"] = cfg.wifi_ap_fallback;
  json["wifi_powersave"] = cfg.wifi_powersave;
  json["wifi_secret"] = cfg.wifi_secret;
  json["tx_frequency"] = String(cfg.tx_frequency,5);
  json["tx_deviation"] = cfg.tx_deviation;
  json["pocsag_tx_frequency"] = String(cfg.pocsag_tx_frequency,5);
  json["pocsag_tx_deviation"] = cfg.pocsag_tx_deviation;
  json["retekess_tx_frequency"] = String(cfg.retekess_tx_frequency,5);
  json["retekess_tx_deviation"] = cfg.retekess_tx_deviation;
  json["retekess_system_id"] = cfg.retekess_system_id;
  json["retekess_alert_type"] = cfg.retekess_alert_type;
  json["tx_power"] = cfg.tx_power;
  json["tx_current_limit"] = cfg.tx_current_limit;
  json["default_mode"] = cfg.default_mode;
  json["pocsag_baud"] = cfg.pocsag_baud;
  json["ota_path"] = cfg.ota_path;
  json["ip_addr"] = cfg.ip_addr;
  json["ip_gw"] = cfg.ip_gw;
  json["ip_netmask"] = cfg.ip_netmask;
  json["ip_dns"] = cfg.ip_dns;
  json["multi_pager_types"] = cfg.multi_pager_types;

  String output;
  serializeJson(json, output);
  return output;
}

boolean parse_settings(DynamicJsonDocument json)
{
    if (json.containsKey("alert_type"))
      cfg.alert_type = json["alert_type"];
    if (json.containsKey("wifi_hostname"))
      strncpy(cfg.wifi_hostname, json["wifi_hostname"],sizeof(cfg.wifi_hostname));
    if (json.containsKey("restaurant_id"))
      cfg.restaurant_id = json["restaurant_id"];
    if (json.containsKey("system_id"))
      cfg.system_id = json["system_id"];
    if (json.containsKey("wifi_ssid"))
      strncpy(cfg.wifi_ssid, json["wifi_ssid"], sizeof(cfg.wifi_ssid));
    if (json.containsKey("wifi_opmode"))
      cfg.wifi_opmode = json["wifi_opmode"];
    if (json.containsKey("wifi_powersave"))
      cfg.wifi_powersave = json["wifi_powersave"];
    if (json.containsKey("wifi_ap_fallback"))
      cfg.wifi_ap_fallback = json["wifi_ap_fallback"];
    if (json.containsKey("wifi_secret"))
      strcpy(cfg.wifi_secret, json["wifi_secret"]);
    if (json.containsKey("tx_frequency"))
      cfg.tx_frequency = json["tx_frequency"];
    if (json.containsKey("tx_deviation"))
      cfg.tx_deviation = json["tx_deviation"];
    if (json.containsKey("pocsag_tx_frequency"))
      cfg.pocsag_tx_frequency = json["pocsag_tx_frequency"];
    if (json.containsKey("pocsag_tx_deviation"))
      cfg.pocsag_tx_deviation = json["pocsag_tx_deviation"];
    if (json.containsKey("retekess_tx_frequency"))
      cfg.retekess_tx_frequency = json["retekess_tx_frequency"];
    if (json.containsKey("retekess_tx_deviation"))
      cfg.retekess_tx_deviation = json["retekess_tx_deviation"];
    if (json.containsKey("retekess_system_id"))
      cfg.retekess_system_id = json["retekess_system_id"];
    if (json.containsKey("retekess_alert_type"))
      cfg.retekess_alert_type = json["retekess_alert_type"];
    if (json.containsKey("tx_power"))
      cfg.tx_power = json["tx_power"];
    if (json.containsKey("tx_current_limit"))
      cfg.tx_current_limit = json["tx_current_limit"];
    if (json.containsKey("pocsag_baud"))
      cfg.pocsag_baud = json["pocsag_baud"];
    if (json.containsKey("default_mode"))
      cfg.default_mode = json["default_mode"];
    if (json.containsKey("ota_path"))
      strncpy(cfg.ota_path, json["ota_path"], sizeof(cfg.ota_path));
    if (json.containsKey("ip_addr"))
      strncpy(cfg.ip_addr, json["ip_addr"], sizeof(cfg.ip_addr));
    if (json.containsKey("ip_gw"))
      strncpy(cfg.ip_gw, json["ip_gw"], sizeof(cfg.ip_gw));
    if (json.containsKey("ip_netmask"))
      strncpy(cfg.ip_netmask, json["ip_netmask"], sizeof(cfg.ip_netmask));
    if (json.containsKey("ip_dns"))
      strncpy(cfg.ip_dns, json["ip_dns"], sizeof(cfg.ip_dns));
    if (json.containsKey("multi_pager_types"))
      cfg.multi_pager_types = json["multi_pager_types"];

    write_config();
    return true;
}

void factory_reset(int state) {
#ifdef HAS_DISPLAY    
    if (state & 1) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 14, "FACTORY");
        display.drawString(64, 42, "RESET");
        d();
    }
#endif
    if (state & 2) {
        info("RESET Config\n");
        cfg.version = 0xff;
        write_config();
        sleep(1);
        ESP.restart();
    }
}
