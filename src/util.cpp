#include "main.h"

int bitset(uint8_t *data, int bitpos, int value, bool reverse) {
    int bytepos = bitpos >> 3;
    bitpos &= 7;
    if(reverse) {
            bitpos = 7  - bitpos;
    }
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

int bitset_rev(uint8_t *data, int bitpos, int value) {
        return bitset(data, bitpos, value, true);
}

int bcd(int number, int *hundreds, int *tens, int *ones) {
    int h = number / 100;
    int t = (number - h * 100) / 10;
    int o = number - h * 100 - t * 10;
    if(hundreds) *hundreds = h;
    if(tens) *tens = t;
    if(ones) *ones = o;
    return h << 8 | t << 4 | o;
}

int reversenibble(int number) {
    return (number & 8) >> 3 | (number & 4) >> 1 | (number & 2) << 1 |
           (number & 1) << 3;
}

float range_check(float val, float min, float max) {
    if(val < min) return min;
    if(val > max) return max;
    return val;
}
