#ifndef NOVA_BOARD_H
#define NOVA_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
void nova_board_init(void);
void nova_board_peripherals_init(void);
void nova_led_set(bool on);
bool nova_led_get(void);
const char *nova_board_name(void);
const char *nova_chip_name(void);
uint32_t nova_sram_capacity_kib(void);
void nova_board_id(char *buffer, size_t capacity);
#endif
