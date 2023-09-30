#include "main.h"

// #define DEBUG
typedef enum {
    TX_IDLE = 0,
    TX_START,
    TX_PREAMBLE,
    TX_SYNC,
    TX_BATCH
} tx_state_t;

typedef struct {
    uint32_t buffer[_MAXTXBATCHES * 8 * 2];
    int num_batches = 0;
    volatile tx_state_t state = TX_IDLE;
    volatile int preamble_counter = 0;
    volatile int batch_counter = 0;
    volatile int cw_counter = 0;
    volatile int bit_counter = 0;
} pocsag_transmitter_t;

static pocsag_transmitter_t tx;

static void IRAM_ATTR onTimer() {
    switch (tx.state) {
        case TX_IDLE:
            break;
        case TX_START:
            tx.preamble_counter = 576;
            tx.state = TX_PREAMBLE;
            break;
        case TX_PREAMBLE:
            tx.preamble_counter--;
            digitalWrite(LoRa_DIO2, !(tx.preamble_counter & 1));

            if (tx.preamble_counter == 0) {
                tx.state = TX_SYNC;
                tx.bit_counter = 32;
                tx.batch_counter = 0;
            }
            break;
        case TX_SYNC:
            tx.bit_counter--;
            digitalWrite(LoRa_DIO2, !((POCSAG_SYNC >> tx.bit_counter) & 1));
            if (tx.bit_counter == 0) {
                tx.state = TX_BATCH;
                tx.cw_counter = 0;
                tx.bit_counter = 32;
            }
            break;
        case TX_BATCH:
            tx.bit_counter--;
            digitalWrite(
                LoRa_DIO2,
                !((tx.buffer[tx.cw_counter + (tx.batch_counter << 4)] >>
                   tx.bit_counter) &
                  1));
            if (tx.bit_counter == 0) {
                if (tx.cw_counter == 15) {
                    tx.batch_counter++;
                    if (tx.batch_counter < tx.num_batches) {
                        tx.state = TX_SYNC;
                        tx.bit_counter = 32;
                    } else {
                        tx.state = TX_IDLE;
                    }
                } else {
                    tx.bit_counter = 32;
                    tx.cw_counter++;
                }
            }
            break;
        default:
            break;
    }
}

inline bool even_parity(uint32_t data) {
    uint32_t temp = data ^ (data >> 16);

    temp = temp ^ (temp >> 8);
    temp = temp ^ (temp >> 4);
    temp = temp ^ (temp >> 2);
    temp = temp ^ (temp >> 1);
    return (temp & 1) ? true : false;
}

static uint32_t crc(uint32_t data) {
    uint32_t ret = data << (BCH_N - BCH_K), shreg = ret;
    uint32_t mask = 1L << (BCH_N - 1), coeff = BCH_POLY << (BCH_K - 1);
    int n = BCH_K;

    for (; n > 0; mask >>= 1, coeff >>= 1, n--)
        if (shreg & mask) shreg ^= coeff;
    ret ^= shreg;
    ret = (ret << 1) | even_parity(ret);
#ifdef DEBUG
    dbg("BCH coder: data: %08x shreg: %08x ret: %08x\n", data, shreg, ret);
#endif
    return ret;
}

static void idlefill(uint32_t *b, size_t len) {
    int j;
    for (j = 0; j < len; j++) b[j] = POCSAG_IDLE;
}

static int poc_beep(uint32_t *buffer, size_t len, uint32_t adr,
                    unsigned function) {
    if (adr > 0x1fffffLU) return 0;

    idlefill(buffer, len >> 2);
    buffer[((adr & 7) << 1)] = crc(((adr >> 1) & 0x1ffffcLU) | (function & 3));
    return 1;
}

static uint32_t fivetol(char *s) {
    static uint32_t chtab[] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9};
    uint32_t ret = 0;
    int i, j;

    for (i = 0; i < 5; i++) {
        j = (4 - i) << 2;
        switch (s[i]) {
            case 0:
            case ' ':
                ret |= 3L << j;
                break;
            case '-':
                ret |= 11L << j;
                break;
            case '[':
                ret |= 15L << j;
                break;
            case ']':
                ret |= 7L << j;
                break;
            case 'U':
                ret |= 13L << j;
                break;
            default:
                if ((s[i] > '0') && (s[i] <= '9'))
                    ret |= chtab[s[i] - '0'] << j;
        }
    }

    return crc(ret | 0x100000UL);
}

static int poc_numeric(uint32_t *buffer, size_t len, uint32_t adr,
                       unsigned function, const char *s) {
    unsigned i = 0, j;
    char msg[21];
    memset(msg, 0, sizeof(msg));
    memccpy(msg, s, 0, sizeof(msg) - 1);

    if (adr > 0x1fffffLU) return 0;

    idlefill(buffer, len >> 2);
    buffer[(j = ((adr & 7) << 1))] =
        crc(((adr >> 1) & 0x1ffffcLU) | (function & 3));
    j++;
    if (msg[0])
        for (i = 0; i < (((strlen(msg) - 1) / 5) + 1); i++)
            buffer[j + i] = fivetol(msg + i * 5);

    return ceil((i + j) * 1.0 / 16.0);
}

static bool getbit(const char *s, int bit) {
    int startbyte = bit / 7, startbit = bit - startbyte * 7;
    return !!(s[startbyte] & (1 << startbit));
}

static uint32_t getword(const char *s, int word) {
    int start = word * 20, i;
    uint32_t ret = 0;
    for (i = 0; i < 20; i++) ret |= getbit(s, start + i) * (0x80000LU >> i);
    return 0x100000LU | ret;
}

static int poc_alphanum(uint32_t *buffer, size_t len, uint32_t adr,
                        unsigned function, const char *s) {
    int i, j, l;

    l = strlen(s);

    if (adr > 0x1fffffLU) return 0;

    idlefill(buffer, len >> 2);
    buffer[(j = ((adr & 7) << 1))] =
        crc(((adr >> 1) & 0x1ffffcLU) | (function & 3));
    j++;

    for (i = 0; i < (l * 7 / 20 + 1); i++) buffer[j + i] = crc(getword(s, i));

    return ceil((i + j) * 1.0 / 16.0);
}

int pocsag_pager(SX1276 fsk, int tx_power, float tx_frequency,
                 float tx_deviation, int baud, uint32_t addr, uint8_t function,
                 func_t telegram_type, const char *msg) {
    size_t len = sizeof(tx.buffer);

    switch (telegram_type) {
        case FUNC_BEEP:
            len = 16 * sizeof(uint32_t);
            tx.num_batches = poc_beep(tx.buffer, len, addr, function);
            break;
        case FUNC_NUM:
            tx.num_batches = poc_numeric(tx.buffer, len, addr, function, msg);
            break;
        case FUNC_ALPHA:
            tx.num_batches = poc_alphanum(tx.buffer, len, addr, function, msg);
            break;
        default:
            break;
    }
#ifdef DEBUG
    dbg("POCSAG:\n");
    dbg("num_batches: %d\n", tx.num_batches);
    for (int i = 0; i < tx.num_batches * 16; i++) {
        printf("%02X ", tx.buffer[i]);
    }
    printf("\n");
#endif

    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000 / baud, true);
    timerAlarmEnable(timer);

    fsk.setOutputPower(tx_power);
    fsk.setFrequency(tx_frequency);
    fsk.setFrequencyDeviation(tx_deviation);
    fsk.setOOK(false);
    fsk.transmitDirect();

    tx.state = TX_START;
    
    while (1) {
        dbg("tx.state           : %d\n", tx.state);
        dbg("tx.batch_counter   : %d\n", tx.batch_counter);
        dbg("tx.cw_counter      : %d\n", tx.cw_counter);
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
