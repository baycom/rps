#include <rps.h>

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
      strcpy(cfg.wifi_ssid, "RPS");
      strcpy(cfg.wifi_secret, "");
      strcpy(cfg.wifi_hostname, "RPS");
      cfg.wifi_opmode = WIFI_ACCESSPOINT;
      cfg.wifi_powersave = false;
      cfg.restaurant_id = 0x0;
      cfg.system_id = 0x0;
      cfg.alert_type = 0x1;
      cfg.default_mode = 0x0;
      cfg.pocsag_baud = 1200;
      cfg.tx_current_limit = 100;
      cfg.tx_power = 17;
      cfg.tx_deviation = 3.5;
      cfg.tx_frequency = 446.146973;
    }
    if(cfg.ota_path[0] == 0xff) {
            cfg.ota_path[0] = 0;
    }
    cfg.version = cfg_ver_num;
    write_config();
  }
  printf("Settings:\n");
  printf("version         : %d\n", cfg.version);
  printf("ssid            : %s\n", cfg.wifi_ssid);
  printf("wifi_secret     : %s\n", cfg.wifi_secret);
  printf("wifi_hostname   : %s\n", cfg.wifi_hostname);
  printf("wifi_opmode     : %d\n", cfg.wifi_opmode);
  printf("wifi_powersave  : %d\n", cfg.wifi_powersave);
  printf("ota_path        : %s\n", cfg.ota_path);
  printf("restaurant_id   : %d\n", cfg.restaurant_id);
  printf("system_id       : %d\n", cfg.system_id);
  printf("alert_type      : %d\n", cfg.alert_type);
  printf("default_mode    : %d\n", cfg.default_mode);
  printf("pocsag_baud     : %d\n", cfg.pocsag_baud);
  printf("tx_frequency    : %.6fMhz\n", cfg.tx_frequency);
  printf("tx_deviation    : %.1fkHz\n", cfg.tx_deviation);
  printf("tx_power        : %ddBm\n", cfg.tx_power);
  printf("tx_current_limit: %dmA\n", cfg.tx_current_limit);
}
