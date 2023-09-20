#include "main.h"

/*
Modulation
Modulation: 2-FSK
Carrier frequency: 433.92 MHz
Baud rate: 10000

Packet structure

Start (1 bit) : 0
Synchronization (12 bytes): 0xAAAAAAAAAAAA
Preamble (8 bytes): 0x1234A30C

Rolling code (1 byte): 0x0 (needs to be different from last transmission)
Separator (2 bytes): 0x01
Pager ID (3 bytes): 0x001 (BCD-coded)

Checksum #1 (1 byte)
Formula: Rolling code + number of significant byte in pager ID - 5 (if result is
smaller than 0, than add 15) Example if rolling code is 2 and pager ID is 0x012:
(2+2-5)+15 = 14 = 0xE

Checksum #2 (1 byte) If first byte of pager ID is larger
than the third byte of pager ID, than: first byte of pager ID - 1
If central byte of pager ID is larger than the two other bytes, than: 0xF
If third byte of pager ID is larger than the first byte of pager ID, than: third
byte of pager ID
- 1 The packet is sent ten times back to back for a total of 1500 bits.
*/

#define NUM_BATCHES 10

static uint8_t sync_word[] = {0x12, 0x34, 0xA3, 0x0C};

int retekess_fsk_prepare(uint8_t *raw, int pager_number) {
    int rolling_code = random(16);
    int hundreds = pager_number / 100;
    int tens = (pager_number-hundreds*100) / 10;
    int ones = pager_number-hundreds*100-tens*10;
    dbg("rolling_code: %d hundreds: %d tens: %d ones: %d\n", rolling_code, tens, ones);
    raw[0] = rolling_code << 4;
    raw[1] = 0x1 << 4 | hundreds;
    raw[2] = tens << 4 | ones;

    int checksum1 = rolling_code + (hundreds?1:0) + (tens?1:0) + (ones?1:0) - 5;
    if (checksum1 < 0) {
        checksum1 += 16;
    }
    
    int checksum2 = 0;
    if (hundreds > ones) {
        checksum2 = hundreds - 1;
    } else if (tens > hundreds && tens > ones) {
        checksum2 = 0xf;
    } else if (ones > hundreds) {
        checksum2 = ones - 1;
    }
    raw[3] = (checksum1&0xf) << 4 | (checksum2&0xf);

#ifdef DEBUG
    printf("raw: ");
    for (int i = 0; i < 4; i++) {
        printf("%02x ", raw[i]);
    }
    printf("\n");
#endif

    return 0;
}

int retekess_fsk_pager(SX1276 fsk, int tx_power, float tx_frequency,
                         float tx_deviation, int restaurant_id, int system_id,
                         int pager_number, int alert_type, bool cancel) {
    uint8_t tx[4];

    dbg("system_id: %d pager_num: %d cancel: %d tx_frequency: %.4f\n",
        system_id, pager_number, cancel, tx_frequency);

    fsk.packetMode();
    fsk.setOutputPower(tx_power);
    fsk.setFrequency(tx_frequency);
    fsk.setFrequencyDeviation(tx_deviation);
    fsk.setBitRate(10.0);
    fsk.setEncoding(RADIOLIB_ENCODING_NRZ);
    fsk.setPreambleLength(48);
    fsk.setPreamblePolarity(RADIOLIB_SX127X_PREAMBLE_POLARITY_AA);
    fsk.setSyncWord(sync_word, sizeof(sync_word));
    fsk.setOOK(false);
    fsk.setCRC(false);

    retekess_fsk_prepare(tx, pager_number);
    int state = 0;
    for(int i=0; i < NUM_BATCHES; i++) {
        state = fsk.transmit(tx, sizeof(tx));

        if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
            dbg("Packet too long!\n");
            break;
        } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
            dbg("Timed out while transmitting!\n");
            break;
        } else if (state != RADIOLIB_ERR_NONE) {
            dbg("Failed to transmit packet, code %d\n", state);
            break;
        }
    }
    return state;
}
