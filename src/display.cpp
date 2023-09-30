#include "main.h"

SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
static unsigned long displayTime = millis();
static unsigned long displayCleared = millis();

void display_setup() {
#ifdef HAS_DISPLAY
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);  // low to reset OLED
    delay(50);
    digitalWrite(OLED_RST, HIGH);  // must be high to turn on OLED

    display.init();
    #ifdef HAS_DISPLAY_UPSIDEDOWN 
        display.flipScreenVertically();
    #endif
#endif
}

void d() {
#ifdef HAS_DISPLAY
    displayTime = millis();
    displayCleared = 0;
    display.display();
#endif
}

void display_loop() {
#ifdef HAS_DISPLAY
    if (cfg.display_timeout > 1000 && (millis() - displayTime > cfg.display_timeout) && !displayCleared) {
        displayCleared = millis();
        display.clear();
        display.display();
    }
#endif
}
