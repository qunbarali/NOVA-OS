#include <stddef.h>
#include <string.h>
#include "nova/sd_spi.h"
#include "nova/spi_bus.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#define SD_CMD0 0u
#define SD_CMD8 8u
#define SD_CMD16 16u
#define SD_CMD17 17u
#define SD_CMD24 24u
#define SD_CMD55 55u
#define SD_CMD58 58u
#define SD_ACMD41 41u
#define SD_TOKEN_START 0xFEu
static uint8_t xfer(nova_sd_card_t *c, uint8_t v) { uint8_t r=0xFFu; (void)spi_write_read_blocking(c->spi,&v,&r,1u); return r; }
static void clocks(nova_sd_card_t *c, unsigned n) { while(n--) (void)xfer(c,0xFFu); }
static uint8_t command(nova_sd_card_t *c,uint8_t cmd,uint32_t arg,uint8_t crc) {
    (void)xfer(c,(uint8_t)(0x40u|cmd)); (void)xfer(c,(uint8_t)(arg>>24)); (void)xfer(c,(uint8_t)(arg>>16));
    (void)xfer(c,(uint8_t)(arg>>8)); (void)xfer(c,(uint8_t)arg); (void)xfer(c,crc);
    for(unsigned i=0;i<10u;i++){uint8_t r=xfer(c,0xFFu);if((r&0x80u)==0u)return r;} return 0xFFu;
}
static uint32_t address(const nova_sd_card_t *c,uint32_t sector){return c->high_capacity?sector:sector*512u;}
static int read_one(nova_sd_card_t *c,uint32_t sector,uint8_t *out) {
    uint8_t r=command(c,SD_CMD17,address(c,sector),0x01u); if(r!=0u)return -1;
    uint32_t wait=100000u; while(wait-- && xfer(c,0xFFu)!=SD_TOKEN_START) { }
    if(wait==0u)return -2;
    for(size_t i=0;i<512u;i++)out[i]=xfer(c,0xFFu);
    (void)xfer(c,0xFFu);(void)xfer(c,0xFFu);(void)xfer(c,0xFFu);
    return 0;
}
static int write_one(nova_sd_card_t *c,uint32_t sector,const uint8_t *in) {
    uint8_t r=command(c,SD_CMD24,address(c,sector),0x01u);if(r!=0u)return -1;
    (void)xfer(c,0xFFu);(void)xfer(c,SD_TOKEN_START);
    for(size_t i=0;i<512u;i++)(void)xfer(c,in[i]);
    (void)xfer(c,0xFFu);(void)xfer(c,0xFFu);
    if((xfer(c,0xFFu)&0x1Fu)!=0x05u)return -2;
    uint32_t wait=500000u;while(wait-- && xfer(c,0xFFu)==0u){} if(wait==0u)return -3;
    return 0;
}
int nova_sd_init(nova_sd_card_t *c,spi_inst_t *spi,uint8_t sck,uint8_t tx,uint8_t rx,uint8_t cs,uint32_t baud) {
    if(!c||!spi)return -1;
    memset(c,0,sizeof(*c));c->spi=spi;c->sck_pin=sck;c->tx_pin=tx;c->rx_pin=rx;c->cs_pin=cs;c->baudrate=baud?baud:400000u;
    (void)spi_init(spi,c->baudrate);gpio_set_function(sck,GPIO_FUNC_SPI);gpio_set_function(tx,GPIO_FUNC_SPI);gpio_set_function(rx,GPIO_FUNC_SPI);
    gpio_init(cs);gpio_set_dir(cs,GPIO_OUT);gpio_put(cs,1);
    if(nova_spi_bus_init(spi)!=0)return -2;
    clocks(c,10u);
    if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
    uint8_t r=command(c,SD_CMD0,0u,0x95u);nova_spi_bus_release(cs);if(r!=1u)return -4;
    if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
    r=command(c,SD_CMD8,0x1AAu,0x87u);
    if(r==1u){uint8_t v[4];for(size_t i=0;i<4u;i++)v[i]=xfer(c,0xFFu);if(v[2]!=0x01u||v[3]!=0xAAu){nova_spi_bus_release(cs);return -5;}}
    nova_spi_bus_release(cs);
    bool ready=false;
    for(unsigned i=0;i<1000u&&!ready;i++){
        if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
        (void)command(c,SD_CMD55,0u,0x01u);r=command(c,SD_ACMD41,(r==1u)?0x40000000u:0u,0x01u);
        nova_spi_bus_release(cs);if(r==0u)ready=true;else sleep_ms(1u);
    }
    if(!ready)return -6;
    if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
    r=command(c,SD_CMD58,0u,0x01u);if(r!=0u){nova_spi_bus_release(cs);return -7;}
    uint8_t ocr[4];for(size_t i=0;i<4u;i++)ocr[i]=xfer(c,0xFFu);
    c->high_capacity=(ocr[0]&0x40u)!=0u;
    if(!c->high_capacity && command(c,SD_CMD16,512u,0x01u)!=0u){nova_spi_bus_release(cs);return -8;}
    nova_spi_bus_release(cs);
    c->initialized=true;c->sector_count=0u;return 0;
}
int nova_sd_read(nova_sd_card_t *c,uint32_t first,uint8_t *buffer,uint32_t count) {
    if(!c||!c->initialized||!buffer||!count||first>UINT32_MAX-count)return -1;
    if(nova_spi_bus_acquire(c->spi,c->cs_pin)!=0)return -2;
    int result=0;for(uint32_t i=0;i<count;i++){result=read_one(c,first+i,buffer+(size_t)i*512u);if(result)break;}
    nova_spi_bus_release(c->cs_pin);return result;
}
int nova_sd_write(nova_sd_card_t *c,uint32_t first,const uint8_t *buffer,uint32_t count) {
    if(!c||!c->initialized||!buffer||!count||first>UINT32_MAX-count)return -1;
    if(nova_spi_bus_acquire(c->spi,c->cs_pin)!=0)return -2;
    int result=0;for(uint32_t i=0;i<count;i++){result=write_one(c,first+i,buffer+(size_t)i*512u);if(result)break;}
    nova_spi_bus_release(c->cs_pin);return result;
}
