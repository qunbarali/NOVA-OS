#include "nova/spi_bus.h"
#include "hardware/gpio.h"
#include "pico/mutex.h"
static mutex_t bus_mutex;
static spi_inst_t *bus_spi;
static bool bus_ready;
int nova_spi_bus_init(spi_inst_t *spi) {
    if (spi == NULL) return -1;
    if (bus_ready) return bus_spi == spi ? 0 : -1;
    mutex_init(&bus_mutex);
    bus_spi = spi;
    bus_ready = true;
    return 0;
}
int nova_spi_bus_acquire(spi_inst_t *spi, uint chip_select_pin) {
    if (!bus_ready || spi != bus_spi) return -1;
    mutex_enter_blocking(&bus_mutex);
    gpio_put(chip_select_pin, 0);
    return 0;
}
void nova_spi_bus_release(uint chip_select_pin) {
    gpio_put(chip_select_pin, 1);
    mutex_exit(&bus_mutex);
}
