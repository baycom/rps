#include "main.h"

int bitset(uint8_t *data, int bitpos, int value) {
    int bytepos = bitpos >> 3;
    bitpos &= 7;
    uint8_t mask = 1 << bitpos;
    switch (value) {
        case 0:
            data[bytepos] &= ~mask;
            break;
        case 1:
            data[bytepos] |= mask;
            break;
        default:
            break;
    }
    return data[bytepos] & mask ? 1 : 0;
}
