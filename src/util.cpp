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

int bcd(int number) {
    int hundreds = number / 100;
    int tens = (number - hundreds * 100) / 10;
    int ones = number - hundreds * 100 - tens * 10;
    return hundreds << 8 | tens << 4 | ones;
}

int reversenibble(int number) {
    return (number & 8) >> 3 | (number & 4) >> 1 | (number & 2) << 1 |
           (number & 1) << 3;
}
