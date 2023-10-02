#include "main.h"

/*
Modulation
Modulation: 2-FSK
Carrier frequency: 433.92 MHz
Baud rate: 10000

Packet structure
                              S
                              E FF     CC
20 Nibbles = 80bit   Preamble Q 12 HTO 12
aaaaaaaaaaaaaaaaaaaa 12340205 4 91 030 90 0000000000

Start: 0-bit
Synchronization (80bit): 0xAAAAAAAAAAAAAAAAAAAA
Preamble (4 nibbles): 0x12340205

Sequence number (1 nibble): 0x0 (needs to be different from last transmission)
Separator (1 nibble): 0x9

Function (1 nibble):
0x1 Paging:
1-999

0x2 programming (reprogram=1)
Step 1: Page 999, Step 2: Page XXX

0x4 mute (alert_type=1)
999: mute all
000: unmute all

Pager ID (3 Nibbles): 0xHTO (BCD-coded)

C1 (1 Nibble) = SEQ + T + T<6?1:2 + Function
C2 (1 Nibble) = H + O
End (40bit): 0000000000

Repeat frame 20 times
*/

typedef enum { TX_IDLE = 0, TX_START, TX_BIT } tx_state_t;

typedef struct {
    volatile tx_state_t state = TX_IDLE;
    uint8_t buffer[64];
    volatile int bits = 0;
    volatile int batch_counter = 0;
    volatile int bit_counter = 0;
} retekess_transmitter_t;

static retekess_transmitter_t tx;
static int rolling_code = 0;

static void IRAM_ATTR onTimer() {
    switch (tx.state) {
        case TX_IDLE:
            break;
        case TX_START:
            tx.state = TX_BIT;
            tx.bit_counter = 0;
            digitalWrite(LoRa_DIO2, 0);
            break;
        case TX_BIT:
            digitalWrite(LoRa_DIO2, bitset_rev(tx.buffer, tx.bit_counter++));
            if (tx.bit_counter == tx.bits) {
                tx.batch_counter--;
                if (tx.batch_counter) {
                    tx.bit_counter = 0;
                    tx.state = TX_START;
                } else {
                    tx.state = TX_IDLE;
                }
            }
            break;
        default:
            break;
    }
}

static int retekess_fsk_td164_prepare(uint8_t *raw, int pager_number,
                                      bool mute_mode = false,
                                      bool prog_mode = false) {
    int hundreds;
    int tens;
    int ones;
    int pager_number_bcd = bcd(pager_number, &hundreds, &tens, &ones);
    dbg("rolling_code: %d hundreds: %d tens: %d ones: %d\n", rolling_code,
        hundreds, tens, ones);

    int i;
    // Sync
    for (i = 0; i < 10; i++) {
        raw[i] = 0xAA;
    }
    // Preamble
    raw[i++] = 0x12;
    raw[i++] = 0x34;
    raw[i++] = 0x02;
    raw[i++] = 0x05;

    // Payload
    rolling_code = (rolling_code + 1) % 16;
    raw[i++] = rolling_code << 4 | 0x9;
    
    int function = 1;
    if (prog_mode) function = 2;
    if (mute_mode) function = 4;

    raw[i++] = function << 4 | hundreds;
    raw[i++] = pager_number_bcd & 0xff;

    int offset = (tens < 6 ? 1 : 2) + function;

    dbg("checksum 1 offset: %d\n", offset);
    int checksum1 = (rolling_code + tens + offset) & 0xf;
    int checksum2 = (hundreds + ones) & 0xf;
    raw[i++] = (checksum1 & 0xf) << 4 | (checksum2 & 0xf);

    for (int j = 0; j < 5; j++) {
        raw[i++] = 0;
    }
#ifdef DEBUG
    printf("raw bytes: ");
    for (int j = 0; j < i; j++) {
        printf("%02x ", raw[j]);
    }
    printf("\nraw %d bits: ", i*8);
    for (int j = 0; j < i*8; j++) {
        printf("%d", bitset_rev(raw, j));
    }
    printf("\nsymbol built\n");
#endif

    return i * 8;
}

int retekess_fsk_td164_pager(SX1276 fsk, int tx_power, float tx_frequency,
                             float tx_deviation, int restaurant_id,
                             int system_id, int pager_number, int alert_type,
                             bool reprogram) {
    dbg("system_id: %d pager_num: %d reprogram: %d tx_frequency: %.4f\n",
        system_id, pager_number, reprogram, tx_frequency);

    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000 / 10000, true);
    timerAlarmEnable(timer);

    fsk.setOutputPower(tx_power);
    fsk.setFrequency(tx_frequency);
    fsk.setFrequencyDeviation(tx_deviation);
    fsk.setOOK(false);
    fsk.transmitDirect();

    memset(tx.buffer, 0, sizeof(tx.buffer));

    int len = retekess_fsk_td164_prepare(tx.buffer, pager_number, alert_type,
                                         reprogram);

    tx.batch_counter = 20;
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
