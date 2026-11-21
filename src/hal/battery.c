#include "nova/battery.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#ifndef NOVA_BATTERY_ADC_GPIO
#define NOVA_BATTERY_ADC_GPIO 26u
#endif
#ifndef NOVA_BATTERY_DIVIDER_NUMERATOR
#define NOVA_BATTERY_DIVIDER_NUMERATOR 0u
#endif
#ifndef NOVA_BATTERY_DIVIDER_DENOMINATOR
#define NOVA_BATTERY_DIVIDER_DENOMINATOR 1u
#endif
int nova_battery_init(void){
#if NOVA_BATTERY_DIVIDER_NUMERATOR == 0
    return -1;
#else
    adc_init();adc_gpio_init(NOVA_BATTERY_ADC_GPIO);return 0;
#endif
}
bool nova_battery_read(nova_battery_info_t *info){
    if(!info)return false;
#if NOVA_BATTERY_DIVIDER_NUMERATOR == 0
    info->available=false;info->millivolts=0u;info->percent=0u;info->low=false;info->critical=false;return false;
#else
    adc_select_input((uint)(NOVA_BATTERY_ADC_GPIO-26u));
    uint32_t raw=adc_read();
    uint32_t pin_mv=(raw*3300u)/4095u;
    uint32_t battery_mv=(pin_mv*NOVA_BATTERY_DIVIDER_DENOMINATOR)/NOVA_BATTERY_DIVIDER_NUMERATOR;
    info->available=true;info->millivolts=(uint16_t)battery_mv;
    info->percent=battery_mv<=3300u?0u:(battery_mv>=4200u?100u:(uint8_t)(((battery_mv-3300u)*100u)/900u));
    info->low=battery_mv<=3500u;info->critical=battery_mv<=3300u;return true;
#endif
}
