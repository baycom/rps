#include "main.h"

struct Button {
    const uint8_t PIN;
    unsigned long handled;
    unsigned long down;
    unsigned long up;
};

Button button1 = {GPIO_BUTTON, 0, false};

void IRAM_ATTR isr() {
    if (!digitalRead(GPIO_BUTTON)) {
        button1.handled = false;
        button1.down = millis();
    } else {
        button1.up = millis();
    }
}

void buttons_setup() {
    pinMode(GPIO_BUTTON, INPUT_PULLUP);
    attachInterrupt(button1.PIN, isr, CHANGE);
}

void buttons_loop() {
    bool released = (button1.up > button1.down) ? true : false;
    unsigned long diff = millis() - button1.down;
    //  printf("down: %ld up: %ld handled: %ld released: %d, diff %d\n",
    //  button1.down, button1.up, button1.handled, released, diff);
    if (button1.down) {
        if (diff > 10000) {
            if (button1.handled != 10000 && !released) {
                button1.handled = 10000;
                factory_reset(1);
            }
            if (released && button1.handled == 10000) {
                factory_reset(2);
            }
        } else if (diff > 1500) {
            if (button1.handled != 1500 && !released) {
                button1.handled = 1500;
                power_off(1);
            }
            if (released && button1.handled == 1500) {
                power_off(2);
            }
        } else if (diff > 100 && released && button1.handled != 100) {
            button1.handled = 100;
            button1.up = 0;
            button1.down = 0;
#ifdef HAS_DISPLAY
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.setFont(ArialMT_Plain_10);
            display.drawString(64, 4, "Version: " + String(VERSION_STR));
            display.drawString(64, 24, "SSID: " + String(cfg.wifi_ssid));
            display.drawString(64, 34, "NAME: " + String(cfg.wifi_hostname));

            String ipStr;
            if(cfg.wifi_opmode == OPMODE_ETH_CLIENT) {
                ipStr = ETH.localIP().toString();
            } else {
                ipStr = WiFi.localIP().toString();
            }
            display.drawString(64, 54, "IP: " + ipStr);
            d();
#endif            
        }
    }
}

