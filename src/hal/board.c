#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "nova/board.h"
#include "nova/device.h"
#include "nova/smp_lock.h"

static bool led_state;
static int led_device_read(void *context, void *buffer, size_t capacity) {
    (void)context;
    if (buffer == NULL || capacity < 1u) return -1;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    ((uint8_t *)buffer)[0] = led_state ? 1u : 0u;
    nova_smp_lock_release(lock);
    return 1;
}
static int led_device_write(void *context, const void *buffer, size_t length) {
    (void)context;
    if (buffer == NULL || length < 1u || ((const uint8_t *)buffer)[0] > 1u) return -1;
    nova_led_set(((const uint8_t *)buffer)[0] != 0u);
    return 1;
}
static const nova_device_ops_t led_device_ops = {
    .read = led_device_read,
    .write = led_device_write,
    .control = NULL
};

void nova_board_init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    led_state = false;
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    const nova_device_t led_device = {
        .id = 0u,
        .name = "led0",
        .device_class = NOVA_DEVICE_GPIO,
        .capabilities = NOVA_DEVICE_CAP_READ | NOVA_DEVICE_CAP_WRITE,
        .ops = &led_device_ops,
        .context = NULL
    };
    (void)nova_device_register(&led_device);
}

void nova_led_set(bool on) {
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    led_state = on;
    gpio_put(PICO_DEFAULT_LED_PIN, on ? 1 : 0);
    nova_smp_lock_release(lock);
}

bool nova_led_get(void) {
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool state = led_state;
    nova_smp_lock_release(lock);
    return state;
}

const char *nova_board_name(void) {
#if PICO_RP2350
    return "Raspberry Pi Pico 2 family";
#elif PICO_RP2040
    return "Raspberry Pi Pico (RP2040)";
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
