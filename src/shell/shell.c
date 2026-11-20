#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "pico/unique_id.h"
#include "nova/shell.h"
#include "nova/kernel.h"
#include "nova/board.h"

#define LINE_CAPACITY 80u
#define NOVA_VERSION "0.1.1"

void nova_shell_write(const char *text) { printf("%s", text); }

static void print_system(void) {
    char id[2u * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1u];
    nova_board_id(id, sizeof(id));
    printf("OS: NOVA Kernel Alpha %s\n", NOVA_VERSION);
    printf("Board: %s\nChip: %s\n", nova_board_name(), nova_chip_name());
    printf("Board ID: %s\n", id);
    printf("SRAM capacity: %lu KiB (total, not free memory)\n", (unsigned long)nova_sram_capacity_kib());
    printf("Core 1 service: %s\n", nova_core1_is_ready() ? "ready" : "starting");
    printf("Uptime: %lu ms\n", (unsigned long)nova_uptime_ms());
}

static void execute(const char *line) {
    if (!strcmp(line, "help")) {
        nova_shell_write("Commands: help, version, system, cpu, memory, tasks, uptime, led on, led off, reboot\n");
    } else if (!strcmp(line, "version")) {
        printf("NOVA Kernel Alpha %s (development)\n", NOVA_VERSION);
    } else if (!strcmp(line, "system")) {
        print_system();
    } else if (!strcmp(line, "cpu")) {
        printf("Chip: %s\nCore 0: shell and kernel foreground\nCore 1: %s\n",
               nova_chip_name(), nova_core1_is_ready() ? "heartbeat service active" : "not ready");
    } else if (!strcmp(line, "memory")) {
        printf("Physical SRAM capacity: %lu KiB\n", (unsigned long)nova_sram_capacity_kib());
        nova_shell_write("Free/used memory accounting is not implemented yet.\n");
    } else if (!strcmp(line, "tasks")) {
        printf("Core 1 heartbeat counter: %lu\n", (unsigned long)nova_task_ticks());
        nova_shell_write("This is a kernel service diagnostic, not a task scheduler.\n");
    } else if (!strcmp(line, "uptime")) {
        printf("%lu ms\n", (unsigned long)nova_uptime_ms());
    } else if (!strcmp(line, "led on")) {
        nova_led_set(true); nova_shell_write("LED on\n");
    } else if (!strcmp(line, "led off")) {
        nova_led_set(false); nova_shell_write("LED off\n");
    } else if (!strcmp(line, "reboot")) {
        nova_shell_write("Rebooting into firmware...\n");
        sleep_ms(100);
        watchdog_reboot(0, 0, 0);
    } else if (line[0] != '\0') {
        nova_shell_write("Unknown command. Type 'help'.\n");
    }
}

void nova_shell_run(void) {
    char line[LINE_CAPACITY];
    size_t used = 0;
    bool overflow = false;
    nova_shell_write("NOVA> ");
    for (;;) {
        int ch = getchar_timeout_us(10000);
        if (ch == PICO_ERROR_TIMEOUT) { tight_loop_contents(); continue; }
        if (ch == '\r' || ch == '\n') {
            line[used] = '\0';
            nova_shell_write("\n");
            if (overflow) nova_shell_write("Input too long; command discarded.\n");
            else execute(line);
            used = 0;
            overflow = false;
            nova_shell_write("NOVA> ");
        } else if ((ch == 8 || ch == 127) && used > 0) {
            --used;
            putchar('\b'); putchar(' '); putchar('\b');
        } else if (ch >= 32 && ch <= 126) {
            if (used < LINE_CAPACITY - 1u) {
                line[used++] = (char)ch;
                putchar(ch);
            } else {
                overflow = true;
            }
        }
    }
}
