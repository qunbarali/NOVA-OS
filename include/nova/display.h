#ifndef NOVA_DISPLAY_H
#define NOVA_DISPLAY_H
#include <stdbool.h>
#include <stdint.h>
#include "hardware/spi.h"
typedef enum { NOVA_DISPLAY_ST7735 = 1, NOVA_DISPLAY_ILI9341 = 2 } nova_display_controller_t;
typedef struct {
    spi_inst_t *spi;
    uint8_t sck_pin, tx_pin, rx_pin, cs_pin, dc_pin, reset_pin;
    int16_t backlight_pin;
    uint16_t width, height, x_offset, y_offset;
    uint32_t baudrate;
    nova_display_controller_t controller;
} nova_display_config_t;
int nova_display_init(const nova_display_config_t *config);
void nova_display_set_backlight(bool enabled);
void nova_display_fill(uint16_t rgb565);
void nova_display_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t rgb565);
void nova_display_pixel(uint16_t x, uint16_t y, uint16_t rgb565);
void nova_display_blit_rgb565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels);
uint16_t nova_display_width(void);
uint16_t nova_display_height(void);
bool nova_display_is_ready(void);
#endif
