#ifndef _POCSAG_H
#define _POCSAG_H

/* ---------------------------------------------------------------------- */

/*
 * the code used by POCSAG is a (n=31,k=21) BCH Code with dmin=5,
 * thus it could correct two bit errors in a 31-Bit codeword.
 * It is a systematic code.
 * The generator polynomial is: 
 *   g(x) = x^10+x^9+x^8+x^6+x^5+x^3+1
 * The parity check polynomial is: 
 *   h(x) = x^21+x^20+x^18+x^16+x^14+x^13+x^12+x^11+x^8+x^5+x^3+1
 * g(x) * h(x) = x^n+1
 */
#define BCH_POLY 03551		/* octal */
#define BCH_N    31
#define BCH_K    21

/*
 * some codewords with special POCSAG meaning
 */
#define POCSAG_SYNC     0x7cd215d8
#define POCSAG_SYNCINFO 0x7cf21436
#define POCSAG_IDLE     0x7a89c197

#define POCSAG_SYNC_WORDS ((2000000 >> 3) << 13)

#define _MAXTXBATCHES   20

typedef enum {
    FUNC_BEEP = 0,
    FUNC_NUM = 1,
    FUNC_ALPHA = 2
} func_t;

int pocsag_setup(void);
int pocsag_pager(SX1276 fsk, int tx_power, float tx_frequency, float tx_deviation, int baud, uint32_t addr, uint8_t function, func_t telegram_type, const char *msg);

/* ---------------------------------------------------------------------- */
#endif /* _POCSAG_H */
