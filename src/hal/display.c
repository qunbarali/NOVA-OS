#include <stddef.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "nova/display.h"
#define CMD_SWRESET 0x01u
#define CMD_SLPOUT 0x11u
#define CMD_DISPON 0x29u
#define CMD_CASET 0x2Au
#define CMD_RASET 0x2Bu
#define CMD_RAMWR 0x2Cu
#define CMD_MADCTL 0x36u
#define CMD_COLMOD 0x3Au
static nova_display_config_t active;
static bool ready;
static void select_display(void) { gpio_put(active.cs_pin, 0); }
static void deselect_display(void) { gpio_put(active.cs_pin, 1); }
static void write_bytes(bool data, const uint8_t *bytes, size_t length) {
    gpio_put(active.dc_pin, data ? 1 : 0); select_display();
    (void)spi_write_blocking(active.spi, bytes, length); deselect_display();
}
static void command(uint8_t value) { write_bytes(false, &value, 1u); }
static void data_bytes(const uint8_t *bytes, size_t length) { if (length) write_bytes(true, bytes, length); }
static void data_byte(uint8_t value) { data_bytes(&value, 1u); }
static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint16_t xa=(uint16_t)(x0+active.x_offset), xb=(uint16_t)(x1+active.x_offset);
    uint16_t ya=(uint16_t)(y0+active.y_offset), yb=(uint16_t)(y1+active.y_offset);
    uint8_t b[4];
    command(CMD_CASET); b[0]=(uint8_t)(xa>>8); b[1]=(uint8_t)xa; b[2]=(uint8_t)(xb>>8); b[3]=(uint8_t)xb; data_bytes(b,4u);
    command(CMD_RASET); b[0]=(uint8_t)(ya>>8); b[1]=(uint8_t)ya; b[2]=(uint8_t)(yb>>8); b[3]=(uint8_t)yb; data_bytes(b,4u);
    command(CMD_RAMWR);
}
static void write_color_run(uint16_t color, uint32_t count) {
    uint8_t b[128];
    for (size_t i=0;i<sizeof b;i+=2u) { b[i]=(uint8_t)(color>>8); b[i+1u]=(uint8_t)color; }
    gpio_put(active.dc_pin,1); select_display();
    while(count) { uint32_t n=count>64u?64u:count; (void)spi_write_blocking(active.spi,b,(size_t)n*2u); count-=n; }
    deselect_display();
}
static void init_st7735(void) {
    command(CMD_SWRESET); sleep_ms(150u); command(CMD_SLPOUT); sleep_ms(120u);
    command(CMD_COLMOD); data_byte(0x05u); command(CMD_MADCTL); data_byte(0xC0u);
    command(0xB1u); { const uint8_t v[]={0x01u,0x2Cu,0x2Du}; data_bytes(v,sizeof v); }
    command(0xB2u); { const uint8_t v[]={0x01u,0x2Cu,0x2Du}; data_bytes(v,sizeof v); }
    command(0xB3u); { const uint8_t v[]={0x01u,0x2Cu,0x2Du,0x01u,0x2Cu,0x2Du}; data_bytes(v,sizeof v); }
    command(0xB4u); data_byte(0x07u);
    command(0xC0u); { const uint8_t v[]={0xA2u,0x02u,0x84u}; data_bytes(v,sizeof v); }
    command(0xC1u); data_byte(0xC5u); command(0xC2u); { const uint8_t v[]={0x0Au,0x00u}; data_bytes(v,sizeof v); }
    command(0xC3u); { const uint8_t v[]={0x8Au,0x2Au}; data_bytes(v,sizeof v); }
    command(0xC4u); { const uint8_t v[]={0x8Au,0xEEu}; data_bytes(v,sizeof v); }
    command(0xC5u); data_byte(0x0Eu);
    command(0xE0u); { const uint8_t v[]={0x0Fu,0x1Au,0x0Fu,0x18u,0x2Fu,0x28u,0x20u,0x22u,0x1Fu,0x1Bu,0x23u,0x37u,0x00u,0x07u,0x02u,0x10u}; data_bytes(v,sizeof v); }
    command(0xE1u); { const uint8_t v[]={0x0Fu,0x1Bu,0x0Fu,0x17u,0x33u,0x2Cu,0x29u,0x2Eu,0x30u,0x30u,0x39u,0x3Fu,0x00u,0x07u,0x03u,0x10u}; data_bytes(v,sizeof v); }
    command(0x13u); sleep_ms(10u); command(CMD_DISPON); sleep_ms(100u);
}
static void init_ili9341(void) {
    command(CMD_SWRESET); sleep_ms(150u);
    command(0xEFu); { const uint8_t v[]={0x03u,0x80u,0x02u}; data_bytes(v,sizeof v); }
    command(0xCFu); { const uint8_t v[]={0x00u,0xC1u,0x30u}; data_bytes(v,sizeof v); }
    command(0xEDu); { const uint8_t v[]={0x64u,0x03u,0x12u,0x81u}; data_bytes(v,sizeof v); }
    command(0xE8u); { const uint8_t v[]={0x85u,0x00u,0x78u}; data_bytes(v,sizeof v); }
    command(0xCBu); { const uint8_t v[]={0x39u,0x2Cu,0x00u,0x34u,0x02u}; data_bytes(v,sizeof v); }
    command(0xF7u); data_byte(0x20u); command(0xEAu); { const uint8_t v[]={0x00u,0x00u}; data_bytes(v,sizeof v); }
    command(0xC0u); data_byte(0x23u); command(0xC1u); data_byte(0x10u);
    command(0xC5u); { const uint8_t v[]={0x3Eu,0x28u}; data_bytes(v,sizeof v); }
    command(0xC7u); data_byte(0x86u); command(CMD_MADCTL); data_byte(0x48u);
    command(CMD_COLMOD); data_byte(0x55u); command(0xB1u); { const uint8_t v[]={0x00u,0x18u}; data_bytes(v,sizeof v); }
    command(0xB6u); { const uint8_t v[]={0x08u,0x82u,0x27u}; data_bytes(v,sizeof v); }
    command(0xF2u); data_byte(0x00u); command(0x26u); data_byte(0x01u);
    command(0xE0u); { const uint8_t v[]={0x0Fu,0x31u,0x2Bu,0x0Cu,0x0Eu,0x08u,0x4Eu,0xF1u,0x37u,0x07u,0x10u,0x03u,0x0Eu,0x09u,0x00u}; data_bytes(v,sizeof v); }
    command(0xE1u); { const uint8_t v[]={0x00u,0x0Eu,0x14u,0x03u,0x11u,0x07u,0x31u,0xC1u,0x48u,0x08u,0x0Fu,0x0Cu,0x31u,0x36u,0x0Fu}; data_bytes(v,sizeof v); }
    command(CMD_SLPOUT); sleep_ms(120u); command(CMD_DISPON); sleep_ms(20u);
}
int nova_display_init(const nova_display_config_t *c) {
    if (!c || !c->spi || !c->width || !c->height ||
        (c->controller!=NOVA_DISPLAY_ST7735 && c->controller!=NOVA_DISPLAY_ILI9341)) return -1;
    if ((c->controller==NOVA_DISPLAY_ST7735 && (c->width>162u || c->height>162u)) ||
        (c->controller==NOVA_DISPLAY_ILI9341 && (c->width>320u || c->height>320u))) return -1;
    active=*c; ready=false;
    (void)spi_init(active.spi,active.baudrate?active.baudrate:12000000u);
    gpio_set_function(active.sck_pin,GPIO_FUNC_SPI); gpio_set_function(active.tx_pin,GPIO_FUNC_SPI); gpio_set_function(active.rx_pin,GPIO_FUNC_SPI);
    gpio_init(active.cs_pin); gpio_set_dir(active.cs_pin,GPIO_OUT); deselect_display();
    gpio_init(active.dc_pin); gpio_set_dir(active.dc_pin,GPIO_OUT); gpio_put(active.dc_pin,1);
    gpio_init(active.reset_pin); gpio_set_dir(active.reset_pin,GPIO_OUT);
    if(active.backlight_pin>=0){gpio_init((uint)active.backlight_pin);gpio_set_dir((uint)active.backlight_pin,GPIO_OUT);gpio_put((uint)active.backlight_pin,1);}
    gpio_put(active.reset_pin,0); sleep_ms(20u); gpio_put(active.reset_pin,1); sleep_ms(120u);
    if(active.controller==NOVA_DISPLAY_ST7735)init_st7735();else init_ili9341();
    ready=true; nova_display_fill(0x0000u); return 0;
}
void nova_display_set_backlight(bool enabled){if(ready && active.backlight_pin>=0)gpio_put((uint)active.backlight_pin,enabled?1:0);}
void nova_display_fill_rect(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color){
    if(!ready||!w||!h||x>=active.width||y>=active.height)return;
    uint32_t xe=(uint32_t)x+w,ye=(uint32_t)y+h;if(xe>active.width)xe=active.width;if(ye>active.height)ye=active.height;
    set_window(x,y,(uint16_t)(xe-1u),(uint16_t)(ye-1u));write_color_run(color,(xe-x)*(ye-y));
}
void nova_display_fill(uint16_t color){if(ready)nova_display_fill_rect(0u,0u,active.width,active.height,color);}
void nova_display_pixel(uint16_t x,uint16_t y,uint16_t color){nova_display_fill_rect(x,y,1u,1u,color);}
uint16_t nova_display_width(void){return ready?active.width:0u;}
uint16_t nova_display_height(void){return ready?active.height:0u;}
bool nova_display_is_ready(void){return ready;}
