#ifndef NOVA_SD_SPI_H
#define NOVA_SD_SPI_H
#include <stdbool.h>
#include <stdint.h>
#include "hardware/spi.h"
#define NOVA_SD_SECTOR_SIZE 512u
#define NOVA_SD_CTRL_GET_INFO 1u
#define NOVA_SD_CTRL_READ_BLOCKS 2u
#define NOVA_SD_CTRL_WRITE_BLOCKS 3u
typedef struct { uint32_t sector_count; uint32_t sector_size; bool high_capacity; } nova_sd_info_t;
typedef struct { uint32_t first_sector; uint32_t sector_count; void *buffer; } nova_sd_transfer_t;
typedef struct {
    spi_inst_t *spi;
    uint8_t sck_pin, tx_pin, rx_pin, cs_pin;
    uint32_t baudrate;
    uint32_t sector_count;
    bool high_capacity;
    bool initialized;
} nova_sd_card_t;
int nova_sd_init(nova_sd_card_t *card, spi_inst_t *spi, uint8_t sck, uint8_t tx, uint8_t rx, uint8_t cs, uint32_t baudrate);
int nova_sd_read(nova_sd_card_t *card, uint32_t first_sector, uint8_t *buffer, uint32_t sector_count);
int nova_sd_write(nova_sd_card_t *card, uint32_t first_sector, const uint8_t *buffer, uint32_t sector_count);
#endif
