#ifndef RETEKESS_OOK_H
#define RETEKESS_OOK_H
int retekess_ook_t115_pager(SX1276 fsk, int tx_power, float tx_frequency,
                         float tx_deviation, int restaurant_id, int system_id,
                         int pager_number, int alert_type, bool cancel = false);
#endif