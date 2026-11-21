#ifndef NOVA_SPI_BUS_H
#define NOVA_SPI_BUS_H
#include "hardware/spi.h"
int nova_spi_bus_init(spi_inst_t *spi);
int nova_spi_bus_acquire(spi_inst_t *spi, uint chip_select_pin);
void nova_spi_bus_release(uint chip_select_pin);
#endif
