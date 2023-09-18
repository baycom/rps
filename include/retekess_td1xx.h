#ifndef RETEKESS_TD1XX_H
#define RETEKESS_TD1XX_H
int retekess_td1xx_pager(SX1276 fsk, int tx_power, float tx_frequency,
                         float tx_deviation, int restaurant_id, int system_id,
                         int pager_number, int alert_type, bool cancel = false);
#endif