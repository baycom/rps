#include "main.h"

SX1276 fsk = new Module(LoRa_CS, LoRa_DIO0, LoRa_RST, LoRa_DIO1);
hw_timer_t *timer = NULL;
static SemaphoreHandle_t xSemaphore;

int pager_setup(void) {    
    dbg("MOSI: %d MISO: %d SCK: %d SS: %d\n", MOSI, MISO, SCK, SS);

    xSemaphore = xSemaphoreCreateBinary();
    if ((xSemaphore) != NULL) {
        xSemaphoreGive(xSemaphore);
    }
    timer = timerBegin(1, 80, true);
    if (!timer) {
        err("timer setup failed\n");
        return -1;
    }
    pinMode(LoRa_DIO2, OUTPUT);
    int state = fsk.beginFSK(cfg.tx_frequency);
    if (state != RADIOLIB_ERR_NONE) {
        info("beginFSK failed, code %d\n", state);
    }
    state |= fsk.setCurrentLimit(cfg.tx_current_limit);
    if (state != RADIOLIB_ERR_NONE) {
        info("setCurrentLimit failed, code %d\n", state);
    }
    state |= fsk.setOutputPower(cfg.tx_power, false);
    if (state != RADIOLIB_ERR_NONE) {
        info("setOutputPower failed, code %d\n", state);
    }

    if (state != RADIOLIB_ERR_NONE) {
        info("beginFSK failed, code %d\n", state);
#ifdef HAS_DISPLAY
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_24);
        display.drawString(64, 12, "SX127X");
        display.drawString(64, 42, "FAIL");
        d();
#endif
    }
    return state;
}

int call_pager(byte mode, int tx_power, float tx_frequency, float tx_deviation,
               int pocsag_baud, int restaurant_id, int system_id,
               int pager_number, int alert_type, bool reprogram_pager,
               func_t pocsag_telegram_type, const char *message, bool cancel) {
    int ret = -1;
    xSemaphoreTake(xSemaphore, portMAX_DELAY);

#ifdef HAS_DISPLAY
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    if (!reprogram_pager) {
        if(!cancel) {
            display.drawString(64, 12, "Paging");
        } else {
            display.drawString(64, 12, "Cancel");
        }
    } else {
        display.drawString(64, 12, "Reprog");
    }
    display.drawString(64, 42, String(pager_number));
    d();
#endif

    if (restaurant_id == -1) restaurant_id = cfg.restaurant_id;

    switch (mode) {
        case 0:
            if (alert_type == -1) alert_type = cfg.alert_type;
            if (tx_frequency == -1) tx_frequency = cfg.tx_frequency;
            if (tx_deviation == -1) tx_deviation = cfg.tx_deviation;
            if (system_id == -1) system_id = cfg.system_id;
            ret = lrs_pager(fsk, tx_power, tx_frequency, tx_deviation,
                            restaurant_id, system_id, pager_number, alert_type,
                            reprogram_pager);
            break;
        case 1:
            if (alert_type == -1) alert_type = cfg.alert_type;
            if (tx_frequency == -1) tx_frequency = cfg.pocsag_tx_frequency;
            if (tx_deviation == -1) tx_deviation = cfg.pocsag_tx_deviation;
            ret = pocsag_pager(fsk, tx_power, tx_frequency, tx_deviation,
                               pocsag_baud, pager_number, alert_type,
                               pocsag_telegram_type, message);
            break;
        case 2:
            if (alert_type == -1) alert_type = cfg.retekess_alert_type;
            if (tx_frequency == -1) tx_frequency = cfg.retekess_tx_frequency;
            if (system_id == -1) system_id = cfg.retekess_system_id;
            ret = retekess_ook_t112_pager(fsk, tx_power, tx_frequency, tx_deviation,
                                      restaurant_id, system_id, pager_number,
                                      alert_type, cancel);
            break;
        case 3:
            if (alert_type == -1) alert_type = cfg.retekess_alert_type;
            if (tx_frequency == -1) tx_frequency = cfg.retekess_tx_frequency;
            if (tx_deviation == -1) tx_deviation = cfg.retekess_tx_deviation;
            if (system_id == -1) system_id = cfg.retekess_system_id;
            ret = retekess_fsk_td164_pager(fsk, tx_power, tx_frequency,
                                        tx_deviation, restaurant_id, system_id,
                                        pager_number, alert_type, reprogram_pager);
            break;
        case 4:
            if (alert_type == -1) alert_type = cfg.retekess_alert_type;
            if (tx_frequency == -1) tx_frequency = cfg.retekess_tx_frequency;
            if (system_id == -1) system_id = cfg.retekess_system_id;
            ret = retekess_ook_td161_pager(fsk, tx_power, tx_frequency,
                                        tx_deviation, restaurant_id, system_id,
                                        pager_number, alert_type);
            break;
        default:
            break;
    }
    xSemaphoreGive(xSemaphore);
    return ret;
}
