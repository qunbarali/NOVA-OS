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
#ifndef NOVA_BATTERY_LOW_MV
#define NOVA_BATTERY_LOW_MV 3500u
#endif
#ifndef NOVA_BATTERY_CRITICAL_MV
#define NOVA_BATTERY_CRITICAL_MV 3300u
#endif
#ifndef NOVA_BATTERY_FULL_MV
#define NOVA_BATTERY_FULL_MV 4200u
#endif
int nova_battery_init(void){
#if NOVA_BATTERY_ADC_GPIO < 26 || NOVA_BATTERY_ADC_GPIO > 28
    return -2;
#elif NOVA_BATTERY_DIVIDER_NUMERATOR == 0
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
    uint32_t raw_total=0u;for(uint8_t i=0u;i<16u;i++)raw_total+=adc_read();
    uint32_t raw=raw_total/16u;
    uint32_t pin_mv=(raw*3300u)/4095u;
    uint32_t battery_mv=(pin_mv*NOVA_BATTERY_DIVIDER_NUMERATOR)/NOVA_BATTERY_DIVIDER_DENOMINATOR;
    info->available=true;info->millivolts=(uint16_t)battery_mv;
    info->percent=battery_mv<=NOVA_BATTERY_CRITICAL_MV?0u:(battery_mv>=NOVA_BATTERY_FULL_MV?100u:(uint8_t)(((battery_mv-NOVA_BATTERY_CRITICAL_MV)*100u)/(NOVA_BATTERY_FULL_MV-NOVA_BATTERY_CRITICAL_MV)));
    info->low=battery_mv<=NOVA_BATTERY_LOW_MV;info->critical=battery_mv<=NOVA_BATTERY_CRITICAL_MV;return true;
#endif
}
