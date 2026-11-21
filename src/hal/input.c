#include <stddef.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "nova/input.h"

static uint8_t button_pins[NOVA_BUTTON_COUNT];
static bool stable_pressed[NOVA_BUTTON_COUNT];
static bool sampled_pressed[NOVA_BUTTON_COUNT];
static uint32_t sample_changed_at[NOVA_BUTTON_COUNT];
static uint8_t pending_events;
static bool initialized;

int nova_input_init(const uint8_t pins[NOVA_BUTTON_COUNT]) {
    if (pins == NULL) return -1;
    for (uint8_t i = 0u; i < NOVA_BUTTON_COUNT; ++i) {
        for (uint8_t j = 0u; j < i; ++j) {
            if (pins[i] == pins[j]) return -1;
        }
    }
    uint32_t now = to_ms_since_boot(get_absolute_time());
    for (uint8_t i = 0u; i < NOVA_BUTTON_COUNT; ++i) {
        button_pins[i] = pins[i];
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
        bool pressed = gpio_get(pins[i]) == 0;
        stable_pressed[i] = pressed;
        sampled_pressed[i] = pressed;
        sample_changed_at[i] = now;
    }
    pending_events = 0u;
    initialized = true;
    return 0;
}

void nova_input_poll(void) {
    if (!initialized) return;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    for (uint8_t i = 0u; i < NOVA_BUTTON_COUNT; ++i) {
        bool pressed = gpio_get(button_pins[i]) == 0;
        if (pressed != sampled_pressed[i]) {
            sampled_pressed[i] = pressed;
            sample_changed_at[i] = now;
            continue;
        }
        if (pressed != stable_pressed[i] &&
            (uint32_t)(now - sample_changed_at[i]) >= NOVA_INPUT_DEBOUNCE_MS) {
            stable_pressed[i] = pressed;
            if (pressed) pending_events |= (uint8_t)(1u << i);
        }
    }
}

uint8_t nova_input_take_events(void) {
    uint8_t events = pending_events;
    pending_events = 0u;
    return events;
}

bool nova_input_is_pressed(nova_button_t button) {
    if (!initialized || (uint8_t)button >= NOVA_BUTTON_COUNT) return false;
    return stable_pressed[(uint8_t)button];
}
