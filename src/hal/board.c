#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/spi.h"
#include "nova/board.h"
#include "nova/device.h"
#include "nova/smp_lock.h"
#include "nova/display.h"
#include "nova/input.h"
#include "nova/sd_spi.h"
#include "nova/fs.h"
#include "nova/usb.h"

#define NOVA_SPI_SCK 18u
#define NOVA_SPI_MOSI 19u
#define NOVA_SPI_MISO 16u
#define NOVA_DISPLAY_CS 13u
#define NOVA_DISPLAY_DC 12u
#define NOVA_DISPLAY_RESET 11u
#define NOVA_DISPLAY_BACKLIGHT 10
#define NOVA_SD_CS 17u
#define NOVA_KEY_UP 2u
#define NOVA_KEY_DOWN 3u
#define NOVA_KEY_SELECT 4u
#define NOVA_KEY_BACK 5u

static bool led_state;
static nova_sd_card_t sd_card;
static bool sd_ready;

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
    .read = led_device_read, .write = led_device_write, .control = NULL
};

static int sd_device_control(void *context, uint32_t command, void *argument) {
    nova_sd_card_t *card = (nova_sd_card_t *)context;
    if (card == NULL || !card->initialized || argument == NULL) return -1;
    if (command == NOVA_SD_CTRL_GET_INFO) {
        nova_sd_info_t *info = (nova_sd_info_t *)argument;
        info->sector_count = card->sector_count;
        info->sector_size = NOVA_SD_SECTOR_SIZE;
        info->high_capacity = card->high_capacity;
        return 0;
    }
    nova_sd_transfer_t *transfer = (nova_sd_transfer_t *)argument;
    if (transfer->buffer == NULL || transfer->sector_count == 0u) return -1;
    if (command == NOVA_SD_CTRL_READ_BLOCKS)
        return nova_sd_read(card, transfer->first_sector, (uint8_t *)transfer->buffer, transfer->sector_count);
    if (command == NOVA_SD_CTRL_WRITE_BLOCKS)
        return nova_sd_write(card, transfer->first_sector, (const uint8_t *)transfer->buffer, transfer->sector_count);
    return -1;
}
static const nova_device_ops_t sd_device_ops = {
    .read = NULL, .write = NULL, .control = sd_device_control
};

void nova_board_peripherals_init(void) {
    const uint8_t key_pins[NOVA_BUTTON_COUNT] = {
        NOVA_KEY_UP, NOVA_KEY_DOWN, NOVA_KEY_SELECT, NOVA_KEY_BACK
    };
    (void)nova_input_init(key_pins);

    nova_display_config_t display = {
        .spi = spi0,
        .sck_pin = NOVA_SPI_SCK, .tx_pin = NOVA_SPI_MOSI, .rx_pin = NOVA_SPI_MISO,
        .cs_pin = NOVA_DISPLAY_CS, .dc_pin = NOVA_DISPLAY_DC, .reset_pin = NOVA_DISPLAY_RESET,
        .backlight_pin = NOVA_DISPLAY_BACKLIGHT,
#ifdef NOVA_DISPLAY_USE_ILI9341
        .width = 240u, .height = 320u, .x_offset = 0u, .y_offset = 0u,
        .controller = NOVA_DISPLAY_ILI9341,
#else
        .width = 128u, .height = 160u, .x_offset = 0u, .y_offset = 0u,
        .controller = NOVA_DISPLAY_ST7735,
#endif
        .baudrate = 12000000u
    };
    (void)nova_display_init(&display);

    sd_ready = nova_sd_init(&sd_card, spi0, NOVA_SPI_SCK, NOVA_SPI_MOSI, NOVA_SPI_MISO, NOVA_SD_CS, 8000000u) == 0;
    nova_usb_bind_storage(&sd_card);
    if (sd_ready) {
        (void)nova_fs_bind(&sd_card);
        const nova_device_t storage = {
            .id = 1u, .name = "sd0", .device_class = NOVA_DEVICE_STORAGE,
            .capabilities = NOVA_DEVICE_CAP_CTRL, .ops = &sd_device_ops, .context = &sd_card
        };
        (void)nova_device_register(&storage);
    }
}

void nova_board_init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    led_state = false;
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    const nova_device_t led_device = {
        .id = 0u, .name = "led0", .device_class = NOVA_DEVICE_GPIO,
        .capabilities = NOVA_DEVICE_CAP_READ | NOVA_DEVICE_CAP_WRITE,
        .ops = &led_device_ops, .context = NULL
    };
    (void)nova_device_register(&led_device);
    nova_board_peripherals_init();
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
