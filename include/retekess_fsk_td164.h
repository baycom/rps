#ifndef RETEKESS_FSK_TD164_H
#define RETEKESS_FSK_TD164_H
int retekess_fsk_td164_pager(SX1276 fsk, int tx_power, float tx_frequency,
                             float tx_deviation, int restaurant_id,
                             int system_id, int pager_number, int alert_type = 0,
                             bool reprogram = false);
#endif