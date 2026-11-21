#ifndef NOVA_BATTERY_H
#define NOVA_BATTERY_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { bool available; uint16_t millivolts; uint8_t percent; bool low; bool critical; } nova_battery_info_t;
/* Disabled unless the board designer confirms the divider and ADC pin. */
int nova_battery_init(void);
bool nova_battery_read(nova_battery_info_t *info);
#endif
