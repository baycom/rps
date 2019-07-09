#include "rps.h"

static size_t generate_lrs_paging_code(byte *telegram, size_t telegram_len, byte restaurant_id, byte system_id, int pager_number, byte alert_type)
{
  if (telegram_len < 15) {
    return -1;
  }

  memset(telegram, 0, telegram_len);

  telegram[0] = 0xAA;
  telegram[1] = 0xAA;
  telegram[2] = 0xAA;
  telegram[3] = 0xFC;
  telegram[4] = 0x2D;
  telegram[5] = restaurant_id;
  telegram[6] = ((system_id << 4) & 0xf0) | ((pager_number >> 8) & 0xf);
  telegram[7] = pager_number;
  telegram[13] = alert_type;
  int crc = 0;
  for (int i = 0; i < 14; i++) {
    crc += telegram[i];
  }
  crc %= 255;
  telegram[14] = crc;

  printf("restaurant_id: %02x\n", restaurant_id);
  printf("system_id    : %02x\n", system_id);
  printf("pager_number : %02x\n", pager_number);
  printf("alert_type   : %02x\n", alert_type);
  printf("crc          : %02x\n", crc);

  return 15;
}

static size_t generate_lrs_reprogramming_code(byte *telegram, size_t telegram_len, byte restaurant_id, byte system_id, int pager_number, byte vibrate)
{
  if (telegram_len < 15) {
    return -1;
  }

  memset(telegram, 0x00, telegram_len);

  telegram[0] = 0xAA;
  telegram[1] = 0xAA;
  telegram[2] = 0xAA;
  telegram[3] = 0xBA;
  telegram[4] = 0x52;
  telegram[5] = restaurant_id;
  telegram[6] = ((system_id << 4) & 0xf0) | ((pager_number >> 8) & 0xf);
  telegram[7] = pager_number;
  telegram[8] = 0xff;
  telegram[9] = 0xff;
  telegram[10] = 0xff;
  telegram[11] = (vibrate<<4)&0x10;
  int crc = 0;
  for (int i = 0; i < 14; i++) {
    crc += telegram[i];
  }
  crc %= 255;
  telegram[14] = crc;

  printf("Reprogram pager:\n");
  printf("restaurant_id: %02x\n", restaurant_id);
  printf("system_id    : %02x\n", system_id);
  printf("pager_number : %02x\n", pager_number);
  printf("vibrate      : %02x\n", vibrate);
  printf("crc          : %02x\n", crc);

  return 15;
}

int lrs_pager(SX1278 fsk, int restaurant_id, int system_id, int pager_number, int alert_type, bool reprogram_pager)
{
  byte txbuf[64];
  size_t len;

  fsk.packetMode();
  fsk.setFrequency(cfg.tx_frequency);
  fsk.setFrequencyDeviation(cfg.tx_deviation);
  fsk.setBitRate(0.622);
  fsk.setDCFree(SX127X_DC_FREE_MANCHESTER);
  fsk.setPreambleLength(0);
  fsk.setSyncWord(NULL,0);

  if(!reprogram_pager) {
    len = generate_lrs_paging_code(txbuf, sizeof(txbuf), restaurant_id, system_id, pager_number, alert_type);
  } else {
    len = generate_lrs_reprogramming_code(txbuf, sizeof(txbuf), restaurant_id, system_id, pager_number, alert_type);
  }
  memcpy(txbuf + len, txbuf, len);
  memcpy(txbuf + len * 2, txbuf, len);

  int state = fsk.transmit(txbuf, len * 3);
  if (state == ERR_NONE) {
    Serial.println(F("Packet transmitted successfully!"));
  }
  else if (state == ERR_PACKET_TOO_LONG) {
    Serial.println(F("Packet too long!"));
  }
  else if (state == ERR_TX_TIMEOUT) {
    Serial.println(F("Timed out while transmitting!"));
  } else {
    Serial.println(F("Failed to transmit packet, code "));
    Serial.println(state);
  }
  return state;
}