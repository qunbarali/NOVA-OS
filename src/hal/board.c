#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "nova/board.h"

static bool led_state;

void nova_board_init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    led_state = false;
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

void nova_led_set(bool on) {
    led_state = on;
    gpio_put(PICO_DEFAULT_LED_PIN, on ? 1 : 0);
}

bool nova_led_get(void) { return led_state; }

const char *nova_board_name(void) {
#if PICO_RP2350
    return "Raspberry Pi Pico 2 family";
#elif defined(PICO_BOARD pico)
    return "Raspberry Pi Pico family";
#else
    return "RP-series compatible board";
#endif
}

const char *nova_chip_name(void) {
#if PICO_RP2350
    return "RP2350 (Cortex-M33 target)";
#elif PICO_RP2040
    return "RP2040 (Cortex-M0+)";
#else
    return "Unknown RP-series chip";
#endif
}

uint32_t nova_sram_capacity_kib(void) {
#if PICO_RP2350
    return 520u;
#elif PICO_RP2040
    return 264u;
#else
    return 0u;
#endif
}

void nova_board_id(char *buffer, size_t capacity) {
    pico_unique_board_id_t id;
    if (buffer == NULL || capacity == 0u) return;
    buffer[0] = '\0';
    pico_get_unique_board_id(&id);
    size_t used = 0;
    for (size_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES && used + 2u < capacity; ++i) {
        int written = snprintf(buffer + used, capacity - used, "%02X", id.id[i]);
        if (written < 0) break;
        used += (size_t)written;
    }
}
