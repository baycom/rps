#include "main.h"

/*
Sync-Pulse            = 1: 220us / 0:6480us	2 x 110 + 59 x 110 = 6710
Sync-Pulse-Data-Start = 6710us
Data-0                = 1: 330us / 0:990us      3 x 110 + 9 x 110 = 1320
Data-1                = 1: 990us / 0:330us      9 x 110 + 1 x 110 = 1320
Data-Data-Start       = 1320us (3030 Bit/s)
LSB First
61 + 24*12 = 349 bits = 43.625 bytes / Frame

Frame:
13 bit System ID
10 bit Pager Number
 1 bit cancel

Repeat frame 12 times
*/

typedef union {
    struct {
        unsigned int system_id : 13;
        unsigned int pager_num : 10;
        unsigned int cancel : 1;
    } s;
    uint8_t b8[3];
} retekess_ook_t;

typedef enum { TX_IDLE = 0, TX_START, TX_BIT } tx_state_t;

typedef struct {
    volatile tx_state_t state = TX_IDLE;
    uint8_t buffer[64];
    volatile int bits = 349;
    volatile int batch_counter = 12;
    volatile int bit_counter = 0;
} retekess_transmitter_t;

static retekess_transmitter_t tx;

static void IRAM_ATTR onTimer() {
    switch (tx.state) {
        case TX_IDLE:
            break;
        case TX_START:
            tx.state = TX_BIT;
            tx.bit_counter = 0;
            break;
        case TX_BIT:
            digitalWrite(LoRa_DIO2, bitset_lsb_first(tx.buffer, tx.bit_counter++));
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

static int symbol_sync(uint8_t *data, int start) {
    bitset_lsb_first(data, start + 0, 1);
    bitset_lsb_first(data, start + 1, 1);
    for (int i = 0; i < 59; i++) {
        bitset_lsb_first(data, start + i + 2, 0);
    }
    return 61;
}
static int symbol_zero(uint8_t *data, int start) {
    for (int i = 0; i < 3; i++) {
        bitset_lsb_first(data, start + i, 1);
    }
    for (int i = 0; i < 9; i++) {
        bitset_lsb_first(data, start + i + 3, 0);
    }
    return 12;
}
static int symbol_one(uint8_t *data, int start) {
    for (int i = 0; i < 9; i++) {
        bitset_lsb_first(data, start + i, 1);
    }
    for (int i = 0; i < 3; i++) {
        bitset_lsb_first(data, start + i + 9, 0);
    }
    return 12;
}

static int retekess_ook_t112_prepare(uint8_t *raw, retekess_ook_t *payload) {
    int pos = 0;
    pos += symbol_sync(raw, 0);
    for (int i = 0; i < 24; i++) {
        if (bitset_lsb_first(payload->b8, i)) {
#ifdef DEBUG
            printf("1");
#endif
            pos += symbol_one(raw, pos);
        } else {
#ifdef DEBUG
            printf("0");
#endif
            pos += symbol_zero(raw, pos);
        }
    }
#ifdef DEBUG
    printf("\nraw: ");
    for (int i = 0; i < pos; i++) {
        printf("%d", bitset_lsb_first(raw, i));
    }
    printf("\nsymbol built\n");
#endif
    return pos;
}

int retekess_ook_t112_pager(SX1276 fsk, int tx_power, float tx_frequency,
                         float tx_deviation, int restaurant_id, int system_id,
                         int pager_number, int alert_type, bool cancel) {
    retekess_ook_t p;
    p.s.system_id = system_id;
    p.s.pager_num = pager_number;
    p.s.cancel = cancel;
    dbg("system_id: %d pager_num: %d cancel: %d tx_frequency: %.4f\n",
        system_id, pager_number, cancel, tx_frequency);
    int len = retekess_ook_t112_prepare(tx.buffer, &p);

    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000 / 9100, true);
    timerAlarmEnable(timer);

    fsk.setOutputPower(tx_power);
    fsk.setFrequency(tx_frequency);
    fsk.setOOK(true);
    fsk.transmitDirect();

    tx.batch_counter = 12;
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
