#ifndef UTIL_H
#define UTIL_H
int bitset(uint8_t *data, int bitpos, int value = -1,  bool reverse = false);
int bitset_rev(uint8_t *data, int bitpos, int value = -1);
int bcd(int number, int *hundreds = NULL, int *tens = NULL, int *ones = NULL);
int reversenibble(int number);
float range_check(float val, float min, float max);
#endif