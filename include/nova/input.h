#ifndef NOVA_INPUT_H
#define NOVA_INPUT_H
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NOVA_BUTTON_UP = 0,
    NOVA_BUTTON_DOWN = 1,
    NOVA_BUTTON_SELECT = 2,
    NOVA_BUTTON_BACK = 3,
    NOVA_BUTTON_COUNT = 4
} nova_button_t;

#define NOVA_BUTTON_EVENT(button) ((uint8_t)(1u << (uint8_t)(button)))
#define NOVA_INPUT_DEBOUNCE_MS 25u

/* Pins are sampled active-low with internal pull-ups enabled. */
int nova_input_init(const uint8_t pins[NOVA_BUTTON_COUNT]);
void nova_input_poll(void);
uint8_t nova_input_take_events(void);
bool nova_input_is_pressed(nova_button_t button);
#endif
