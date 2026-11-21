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
static bool wait_data_token(nova_sd_card_t *c) { for (unsigned i=0u; i<100000u; ++i) if (xfer(c,0xFFu)==SD_TOKEN_START) return true; return false; }
static void clocks(nova_sd_card_t *c, unsigned n) { while(n--) (void)xfer(c,0xFFu); }
static uint8_t command(nova_sd_card_t *c,uint8_t cmd,uint32_t arg,uint8_t crc) {
    (void)xfer(c,(uint8_t)(0x40u|cmd)); (void)xfer(c,(uint8_t)(arg>>24)); (void)xfer(c,(uint8_t)(arg>>16));
    (void)xfer(c,(uint8_t)(arg>>8)); (void)xfer(c,(uint8_t)arg); (void)xfer(c,crc);
    for(unsigned i=0;i<10u;i++){uint8_t r=xfer(c,0xFFu);if((r&0x80u)==0u)return r;} return 0xFFu;
}
static uint32_t address(const nova_sd_card_t *c,uint32_t sector){return c->high_capacity?sector:sector*512u;}
static int read_one(nova_sd_card_t *c,uint32_t sector,uint8_t *out) {
    uint8_t r=command(c,SD_CMD17,address(c,sector),0x01u); if(r!=0u)return -1;
    if (!wait_data_token(c)) return -2;
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
    bool ready=false; for(unsigned i=0u;i<500000u;i++){if(xfer(c,0xFFu)!=0u){ready=true;break;}} if(!ready)return -3;
    return 0;
}
static uint32_t read_capacity_sectors(nova_sd_card_t *c) {
    if (command(c, 9u, 0u, 0x01u) != 0u) return 0u;
    if (!wait_data_token(c)) return 0u;
    uint8_t csd[16];
    for (size_t i = 0u; i < sizeof(csd); ++i) csd[i] = xfer(c, 0xFFu);
    (void)xfer(c, 0xFFu); (void)xfer(c, 0xFFu);
    if ((csd[0] >> 6) == 1u) {
        uint32_t size = ((uint32_t)(csd[7] & 0x3Fu) << 16) | ((uint32_t)csd[8] << 8) | csd[9];
        return (size + 1u) * 1024u;
    }
    uint32_t read_len = (uint32_t)(csd[5] & 0x0Fu);
    uint32_t size = ((uint32_t)(csd[6] & 0x03u) << 10) | ((uint32_t)csd[7] << 2) | ((uint32_t)(csd[8] & 0xC0u) >> 6);
    uint32_t mult = ((uint32_t)(csd[9] & 0x03u) << 1) | ((uint32_t)(csd[10] & 0x80u) >> 7);
    uint64_t bytes = ((uint64_t)size + 1u) << (mult + 2u + read_len);
    return (uint32_t)(bytes / 512u);
}
int nova_sd_init(nova_sd_card_t *c,spi_inst_t *spi,uint8_t sck,uint8_t tx,uint8_t rx,uint8_t cs,uint32_t baud) {
    if(!c||!spi)return -1;
    memset(c,0,sizeof(*c));c->spi=spi;c->sck_pin=sck;c->tx_pin=tx;c->rx_pin=rx;c->cs_pin=cs;c->baudrate=baud?baud:12000000u;
    (void)spi_init(spi,400000u);gpio_set_function(sck,GPIO_FUNC_SPI);gpio_set_function(tx,GPIO_FUNC_SPI);gpio_set_function(rx,GPIO_FUNC_SPI);
    gpio_init(cs);gpio_set_dir(cs,GPIO_OUT);gpio_put(cs,1);
    if(nova_spi_bus_init(spi)!=0)return -2;
    clocks(c,10u);
    if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
    uint8_t r=command(c,SD_CMD0,0u,0x95u);nova_spi_bus_release(cs);if(r!=1u)return -4;
    if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
    r=command(c,SD_CMD8,0x1AAu,0x87u);
    bool version2 = (r == 1u);
    if(version2){uint8_t v[4];for(size_t i=0;i<4u;i++)v[i]=xfer(c,0xFFu);if(v[2]!=0x01u||v[3]!=0xAAu){nova_spi_bus_release(cs);return -5;}}
    nova_spi_bus_release(cs);
    bool ready=false;
    for(unsigned i=0;i<1000u&&!ready;i++){
        if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
        (void)command(c,SD_CMD55,0u,0x01u);r=command(c,SD_ACMD41,version2?0x40000000u:0u,0x01u);
        nova_spi_bus_release(cs);if(r==0u)ready=true;else sleep_ms(1u);
    }
    if(!ready)return -6;
    if(nova_spi_bus_acquire(spi,cs)!=0)return -3;
    r=command(c,SD_CMD58,0u,0x01u);if(r!=0u){nova_spi_bus_release(cs);return -7;}
    uint8_t ocr[4];for(size_t i=0;i<4u;i++)ocr[i]=xfer(c,0xFFu);
    c->high_capacity=(ocr[0]&0x40u)!=0u;
    if(!c->high_capacity && command(c,SD_CMD16,512u,0x01u)!=0u){nova_spi_bus_release(cs);return -8;}
    c->sector_count = read_capacity_sectors(c);
    nova_spi_bus_release(cs);
    if (c->sector_count == 0u || c->sector_count > NOVA_SD_MAX_CAPACITY_SECTORS) return -9;
    (void)spi_set_baudrate(spi, c->baudrate);
    c->initialized=true;return 0;
}
int nova_sd_read(nova_sd_card_t *c,uint32_t first,uint8_t *buffer,uint32_t count) {
    if(!c||!c->initialized||!buffer||!count||first>UINT32_MAX-count||first+count>c->sector_count)return -1;
    if(nova_spi_bus_acquire(c->spi,c->cs_pin)!=0)return -2;
    (void)spi_set_baudrate(c->spi,c->baudrate);
    int result=0;for(uint32_t i=0;i<count;i++){result=read_one(c,first+i,buffer+(size_t)i*512u);if(result)break;}
    nova_spi_bus_release(c->cs_pin);return result;
}
int nova_sd_write(nova_sd_card_t *c,uint32_t first,const uint8_t *buffer,uint32_t count) {
    if(!c||!c->initialized||!buffer||!count||first>UINT32_MAX-count||first+count>c->sector_count)return -1;
    if(nova_spi_bus_acquire(c->spi,c->cs_pin)!=0)return -2;
    (void)spi_set_baudrate(c->spi,c->baudrate);
    int result=0;for(uint32_t i=0;i<count;i++){result=write_one(c,first+i,buffer+(size_t)i*512u);if(result)break;}
    nova_spi_bus_release(c->cs_pin);return result;
}
