#ifndef PAGER_H
#define PAGER_H
typedef struct {
  int restaurant_id; 
  int system_id; 
  int pager_number; 
  int alert_type;
} pager_t;

extern hw_timer_t *timer;
extern SX1276 fsk;

int pager_setup(void);
int call_pager(byte mode, int tx_power, float tx_frequency, float tx_deviation,
               int pocsag_baud, int restaurant_id, int system_id,
               int pager_number, int alert_type, bool reprogram_pager,
               func_t pocsag_telegram_type, const char *message, bool cancel);

#endif