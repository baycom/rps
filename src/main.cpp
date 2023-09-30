#include "main.h"

settings_t cfg;
bool eth_connected = false;
static EOTAUpdate *updater;

void WiFiEvent(WiFiEvent_t event) {
    dbg("WiFiEvent: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            dbg("ETH Started\n");
            // set eth hostname here
            ETH.setHostname(cfg.wifi_hostname);
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            dbg("ETH Connected\n");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            info("ETH MAC: %s, IPv4: %s (%s, %dMbps)\n",
                 ETH.macAddress().c_str(), ETH.localIP().toString().c_str(),
                 ETH.fullDuplex() ? "FULL_DUPLEX" : "HALF_DUPLEX",
                 ETH.linkSpeed());
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            if (!eth_connected) {
                if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                    info("WiFi MAC: %s, IPv4: %s\n", WiFi.macAddress().c_str(),
                         WiFi.localIP().toString().c_str());
                }
#ifdef HAS_DISPLAY
                display.setFont(ArialMT_Plain_10);
                display.drawString(64, 54, "IP: " + WiFi.localIP().toString());
                d();
#endif
                if (!MDNS.begin(cfg.wifi_hostname)) {
                    err("MDNS responder failed to start\n");
                }
                eth_connected = true;
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            dbg("ETH Disconnected\n");
            //            eth_connected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            dbg("ETH Stopped\n");
            //            eth_connected = false;
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    display_setup();
    info("Version: %s, Version Number: %d, CFG Number: %d\n", VERSION_STR,
         VERSION_NUMBER, cfg_ver_num);
    info("Initializing ... ");

    EEPROM.begin(EEPROM_SIZE);
    read_config();
    WiFi.onEvent(WiFiEvent);
    buttons_setup();

    pinMode(GPIO_NUM_0, INPUT_PULLUP);
    
#ifdef HAS_DISPLAY
    display.init();
    display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.clear();
    display.drawString(64, 4, "Restaurant Paging Service");
    display.drawString(64, 14, "Version: " + String(VERSION_STR));
    String modeStr = "";
    switch (cfg.wifi_opmode) {
        case 0:
            modeStr = "AP";
            break;
        case 1:
            modeStr = "STA";
            break;
        case 2:
            modeStr = "ETH";
            break;
    }
    display.drawString(64, 24, "Mode: " + modeStr);
    if (cfg.wifi_opmode < 2) {
        display.drawString(64, 34, "SSID: " + String(cfg.wifi_ssid));
    }
    display.drawString(64, 44, "NAME: " + String(cfg.wifi_hostname));

    d();
#endif

    updater = new EOTAUpdate(cfg.ota_path, VERSION_NUMBER, 3600000UL, "RPS/" VERSION_STR);
    pager_setup();

    if (cfg.wifi_opmode == OPMODE_ETH_CLIENT) {
    #ifdef LILYGO_POE    
        pinMode(NRST, OUTPUT);
        for(int i=0;i<4;i++) {
            digitalWrite(NRST, i&1);
            delay(200);
        }
    #endif
    ETH.begin();
//    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
    } else if (cfg.wifi_opmode == OPMODE_WIFI_STATION) {
        WiFi.disconnect();
        WiFi.setAutoReconnect(true);
        WiFi.setHostname(cfg.wifi_hostname);
        WiFi.setSleep(cfg.wifi_powersave);
        WiFi.mode(WIFI_STA);

        IPAddress myIP;
        IPAddress myGW;
        IPAddress myNM;
        IPAddress myDNS;

        myIP.fromString(cfg.ip_addr);
        myGW.fromString(cfg.ip_gw);
        myNM.fromString(cfg.ip_netmask);
        myDNS.fromString(cfg.ip_dns);

        WiFi.config(myIP, myGW, myNM, myDNS);
        WiFi.begin(cfg.wifi_ssid, cfg.wifi_secret);

        info("\n");
    } else if (cfg.wifi_opmode == OPMODE_WIFI_ACCESSPOINT) {
        WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
        IPAddress IP = WiFi.softAPIP();
        info("AP IP address: %s\n", IP.toString().c_str());
#ifdef HAS_DISPLAY        
        display.setFont(ArialMT_Plain_10);
        for (int x = 0; x < 128; x++) {
            for (int y = 0; y < 20; y++) {
                display.clearPixel(x, y + 24);
            }
        }
        display.drawString(
            64, 24,
            "WIFI: " + String((cfg.wifi_opmode == OPMODE_WIFI_STATION) ? "STA"
                                                                       : "AP"));
        display.drawString(64, 34, "SSID: " + String(cfg.wifi_ssid));
        display.drawString(64, 54, "IP: " + IP.toString());
        d();
#endif        
    }

    webserver_setup();
}

void power_off(int state) {
#ifdef HAS_DISPLAY    
    if (state & 1) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 32, "Power off.");
        d();
    }
#endif
    if (state & 2) {
#ifdef HAS_DISPLAY
        display.clear();
        digitalWrite(OLED_RST, LOW);  // low to reset OLED
#endif
#ifdef HELTEC
        digitalWrite(Vext, HIGH);
#endif
        esp_deep_sleep_start();
    }
}

void wifi_loop() {
    static unsigned long last_blink = 0;
    static int count = 0;

    if (!eth_connected && millis() < 30000) {
        if (cfg.wifi_opmode == OPMODE_WIFI_STATION &&
            cfg.wifi_ap_fallback == 1 && WiFi.status() != WL_CONNECTED) {
            uint8_t mac[10];
            WiFi.macAddress(mac);
            sprintf(cfg.wifi_ssid, "RPS-%02X%02X%02X", mac[3], mac[4], mac[5]);
            warn("\nFailed to connect to SSID %s falling back to AP mode\n",
                 cfg.wifi_ssid);
            cfg.wifi_secret[0] = 0;
            cfg.wifi_opmode = OPMODE_WIFI_ACCESSPOINT;
            WiFi.disconnect();
            WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
            IPAddress IP = WiFi.softAPIP();
            info("AP IP address: %s\n", IP.toString().c_str());
        }
        if ((millis() - last_blink) > 500) {
            last_blink = millis();
            count++;
            info("%d\n", count);
        }
    }
}

void loop() {
#ifdef HAS_DISPLAY
    display_loop();
#endif
    wifi_loop();
    buttons_loop();

    if (cfg.ota_path[0] && cfg.wifi_opmode && eth_connected) {
        updater->CheckAndUpdate();
    }
}
