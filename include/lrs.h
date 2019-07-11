#ifndef _LRS_H
#define _LRS_H
int lrs_pager(SX1278 fsk, int tx_power, float tx_frequency, float tx_deviation, int restaurant_id, int system_id, int pager_number, int alert_type, bool reprogram_pager);
#endif