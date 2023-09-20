#include "main.h"

typedef enum { TX_IDLE = 0, TX_START, TX_BIT } tx_state_t;

typedef struct {
    volatile tx_state_t state = TX_IDLE;
    uint8_t buffer[256];
    volatile int bits = 349;
    volatile int batch_counter = 12;
    volatile int bit_counter = 0;
} retekess_transmitter_t;

static retekess_transmitter_t tx;

static int symbol_zero(uint8_t *data, int start) {
    bitset(data, start, 1);
    for (int i = 0; i < 3; i++) {
        bitset(data, start + i + 1, 0);
    }
    return 4;
}
static int symbol_one(uint8_t *data, int start) {
    for (int i = 0; i < 3; i++) {
        bitset(data, start + i, 1);
    }
    bitset(data, start + 3, 0);
    return 4;
}
static int symbol_off(uint8_t *data, int start) {
    for (int i = 0; i < 4; i++) {
        bitset(data, start + i, 0);
    }
    return 4;
}

static int retekess_ook_td158_prepare(uint8_t *raw, int system_id,
                                      int pager_number, int alert_type) {
    int pos = 0;
    uint8_t frame[16];
    int system_id_bcd = bcd(system_id);
    int pager_number_bcd = bcd(pager_number);
    memset(frame, 0, sizeof(frame));
    frame[0] = (pager_number_bcd)&0xf;
    frame[1] = (pager_number_bcd >> 4) & 0xf;
    frame[2] = (pager_number_bcd >> 8) & 0xf;
    frame[3] = alert_type & 0xf;
    frame[4] = (system_id_bcd)&0xf;
    frame[5] = (system_id_bcd >> 4) & 0xf;
    frame[6] = (system_id_bcd >> 8) & 0xf;
    
#ifdef DEBUG
    printf("\nframe: ");
    for (int i = 0; i < sizeof(frame); i++) {
        printf("%02x ", frame[i]);
    }
#endif

    for (int i = 0; i < 9; i++) {
        for (int b = 0; b < 4; b++) {
            if (bitset(frame + i, b)) {
                pos += symbol_one(raw, pos);
            } else {
                pos += symbol_zero(raw, pos);
            }
        }
    }
    for (int i = 9; i < sizeof(frame); i++) {
        pos += symbol_off(raw, pos);
    }

#ifdef DEBUG
    printf("\nraw %d bits: ", pos);
    for (int i = 0; i < pos; i++) {
        printf("%d", bitset(raw, i));
    }
    printf("\nsymbol built\n");
#endif
    return pos;
}

static void IRAM_ATTR onTimer() {
    switch (tx.state) {
        case TX_IDLE:
            break;
        case TX_START:
            tx.state = TX_BIT;
            tx.bit_counter = 0;
            break;
        case TX_BIT:
            digitalWrite(LoRa_DIO2, bitset(tx.buffer, tx.bit_counter++));
            if (tx.bit_counter == tx.bits) {
                tx.batch_counter--;
                if (tx.batch_counter) {
                    tx.bit_counter = 0;
                } else {
                    tx.state = TX_IDLE;
                }
            }
            break;
        default:
            break;
    }
}

int retekess_ook_td158_pager(SX1276 fsk, int tx_power, float tx_frequency,
                             float tx_deviation, int restaurant_id,
                             int system_id, int pager_number, int alert_type) {
    dbg("system_id: %d pager_num: %d alert_type: %d tx_frequency: %.4f\n",
        system_id, pager_number, alert_type, tx_frequency);

    int len = retekess_ook_td158_prepare(tx.buffer, system_id, pager_number,
                                         alert_type);

    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000 / 5000, true);
    timerAlarmEnable(timer);

    fsk.setOutputPower(tx_power);
    fsk.setFrequency(tx_frequency);
    fsk.setOOK(true);
    fsk.transmitDirect();

    tx.batch_counter = 30;
    tx.bits = len;
    tx.state = TX_START;

    while (1) {
        dbg("tx.state           : %d\n", tx.state);
        dbg("tx.batch_counter   : %d\n", tx.batch_counter);
        dbg("tx.bit_counter     : %d\n", tx.bit_counter);
        bool done = false;
        if (tx.state == TX_IDLE) {
            done = true;
        }
        if (done) {
            break;
        }
        usleep(100000);
    }
    fsk.standby();
    timerAlarmDisable(timer);
    timerDetachInterrupt(timer);
    return 0;
}
