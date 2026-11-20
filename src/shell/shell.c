#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "pico/unique_id.h"
#include "nova/shell.h"
#include "nova/kernel.h"
#include "nova/board.h"
#include "nova/task.h"
#include "nova/memory.h"

#define LINE_CAPACITY 80u
#define NOVA_VERSION "0.2.0"

typedef struct { bool led_on; uint32_t transitions; } demo_context_t;
static demo_context_t demo_context;
static nova_task_id_t demo_task_id = NOVA_TASK_ID_INVALID;

void nova_shell_write(const char *text) { printf("%s", text); }

static const char *task_state_name(nova_task_state_t state) {
    switch (state) {
        case NOVA_TASK_READY: return "READY";
        case NOVA_TASK_RUNNING: return "RUNNING";
        case NOVA_TASK_SLEEPING: return "SLEEP";
        case NOVA_TASK_SUSPENDED: return "SUSPEND";
        default: return "UNUSED";
    }
}

static nova_task_result_t demo_led_task(void *context) {
    demo_context_t *demo = (demo_context_t *)context;
    demo->led_on = !demo->led_on;
    ++demo->transitions;
    nova_led_set(demo->led_on);
    nova_task_sleep_current(500u);
    return NOVA_TASK_AGAIN;
}

static void print_system(void) {
    char id[2u * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1u];
    nova_board_id(id, sizeof(id));
    printf("OS: NOVA Kernel Alpha %s\n", NOVA_VERSION);
    printf("Board: %s\nChip: %s\n", nova_board_name(), nova_chip_name());
    printf("Board ID: %s\n", id);
    printf("SRAM capacity: %lu KiB (total, not free memory)\n", (unsigned long)nova_sram_capacity_kib());
    printf("Core 1 service: %s\n", nova_core1_is_ready() ? "ready" : "starting");
    printf("Uptime: %lu ms; system ticks: %lu; timer IRQ: %s\n",
           (unsigned long)nova_uptime_ms(), (unsigned long)nova_system_ticks(),
           nova_kernel_timer_ready() ? "active" : "failed");
}

static void print_tasks(void) {
    nova_task_info_t info[NOVA_TASK_MAX];
    size_t count = nova_task_snapshot(info, NOVA_TASK_MAX);
    printf("Tasks: %lu/%u; dispatches: %lu\n", (unsigned long)count,
           (unsigned)NOVA_TASK_MAX, (unsigned long)nova_task_dispatch_count());
    for (size_t i = 0u; i < count; ++i) {
        printf("  #%u %-12s %-8s runs=%lu\n", (unsigned)info[i].id, info[i].name,
               task_state_name(info[i].state), (unsigned long)info[i].runs);
    }
}

static void execute(const char *line) {
    if (!strcmp(line, "help")) {
        nova_shell_write("Commands: help, version, system, cpu, memory, tasks, uptime, task demo, task stop, led on, led off, reboot\n");
    } else if (!strcmp(line, "version")) {
        printf("NOVA Kernel Alpha %s (development)\n", NOVA_VERSION);
    } else if (!strcmp(line, "system")) {
        print_system();
    } else if (!strcmp(line, "cpu")) {
        printf("Chip: %s\nCore 0: scheduler and shell\nCore 1: %s\n",
               nova_chip_name(), nova_core1_is_ready() ? "heartbeat service active" : "not ready");
    } else if (!strcmp(line, "memory")) {
        printf("SRAM capacity: %lu KiB; fixed pool: %lu/%u blocks free\n",
               (unsigned long)nova_sram_capacity_kib(),
               (unsigned long)nova_pool_free_blocks(), (unsigned)NOVA_POOL_BLOCK_COUNT);
        nova_shell_write("Pool blocks are 32 bytes; general heap accounting is not implemented.\n");
    } else if (!strcmp(line, "tasks")) {
        print_tasks();
    } else if (!strcmp(line, "uptime")) {
        printf("%lu ms\n", (unsigned long)nova_uptime_ms());
    } else if (!strcmp(line, "task demo")) {
        if (demo_task_id != NOVA_TASK_ID_INVALID) {
            nova_shell_write("LED demo task already exists.\n");
        } else {
            demo_context.led_on = false;
            demo_context.transitions = 0u;
            demo_task_id = nova_task_create("ledpulse", demo_led_task, &demo_context);
            printf("LED demo task: %s (id=%u)\n",
                   demo_task_id == NOVA_TASK_ID_INVALID ? "creation failed" : "started",
                   (unsigned)demo_task_id);
        }
    } else if (!strcmp(line, "task stop")) {
        if (demo_task_id == NOVA_TASK_ID_INVALID || !nova_task_terminate(demo_task_id)) {
            nova_shell_write("No stoppable LED demo task.\n");
        } else {
            demo_task_id = NOVA_TASK_ID_INVALID;
            nova_led_set(false);
            nova_shell_write("LED demo task stopped.\n");
        }
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
    size_t used = 0u;
    bool overflow = false;
    nova_shell_write("NOVA> ");
    for (;;) {
        nova_kernel_poll();
        int ch = getchar_timeout_us(1000);
        if (ch == PICO_ERROR_TIMEOUT) { tight_loop_contents(); continue; }
        if (ch == '\r' || ch == '\n') {
            line[used] = '\0';
            nova_shell_write("\n");
            if (overflow) nova_shell_write("Input too long; command discarded.\n");
            else execute(line);
            used = 0u;
            overflow = false;
            nova_shell_write("NOVA> ");
        } else if ((ch == 8 || ch == 127) && used > 0u) {
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
