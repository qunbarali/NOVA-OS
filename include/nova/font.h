#ifndef NOVA_FONT_H
#define NOVA_FONT_H
#include <stdint.h>
void nova_font_draw_char(uint16_t x, uint16_t y, char value, uint16_t foreground, uint16_t background);
void nova_font_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t foreground, uint16_t background);
#endif
