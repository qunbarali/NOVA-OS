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
#include "nova/crash.h"
#include "nova/device.h"
#include "nova/ui.h"
#include "nova/usb.h"
#include "nova/app_manager.h"

#define LINE_CAPACITY 80u
#define NOVA_VERSION "1.5.0"

typedef struct { bool led_on; uint32_t transitions; } demo_context_t;
static demo_context_t demo_context;
static nova_task_id_t demo_task_id = NOVA_TASK_ID_INVALID;

void nova_shell_write(const char *text) { printf("%s", text); }

static const char *task_state_name(nova_task_state_t state) {
    switch (state) {
        case NOVA_TASK_READY: return "READY";
        case NOVA_TASK_RUNNING: return "RUNNING";
        case NOVA_TASK_SLEEPING: return "SLEEP";
        case NOVA_TASK_BLOCKED: return "BLOCKED";
        case NOVA_TASK_SUSPENDED: return "SUSPEND";
        case NOVA_TASK_FAULTED: return "FAULTED";
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
    printf("Tasks: %lu/%u; dispatches: %lu; faults: %lu\n", (unsigned long)count,
           (unsigned)NOVA_TASK_MAX, (unsigned long)nova_task_dispatch_count(),
           (unsigned long)nova_task_fault_count());
    for (size_t i = 0u; i < count; ++i) {
        printf("  #%u %-12s %-8s prio=%u runs=%lu\n", (unsigned)info[i].id, info[i].name,
               task_state_name(info[i].state), (unsigned)info[i].priority,
               (unsigned long)info[i].runs);
        printf("     stack=%u/%u bytes (high-water; limit=%u)\n", (unsigned)info[i].stack_used_bytes,
               (unsigned)info[i].stack_size_bytes, (unsigned)NOVA_TASK_STACK_LIMIT_BYTES);
    }
}

static void execute(const char *line) {
    if (!strcmp(line, "help")) {
        nova_shell_write("Commands: help, version, system, cpu, memory, tasks, faults, crash, devices, device led0 on/off, uptime, app status, app stop, task demo, task stop, led on, led off, reboot\n");
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
    } else if (!strcmp(line, "faults")) {
        nova_fault_record_t records[NOVA_FAULT_LOG_CAPACITY];
        size_t count = nova_task_fault_snapshot(records, NOVA_FAULT_LOG_CAPACITY);
        printf("Task fault journal: %lu/%u retained; total faults: %lu\n",
               (unsigned long)count, (unsigned)NOVA_FAULT_LOG_CAPACITY,
               (unsigned long)nova_task_fault_count());
        for (size_t i = 0u; i < count; ++i) {
            const char *reason = records[i].reason == NOVA_FAULT_STACK_GUARD ? "STACK_GUARD" :
                records[i].reason == NOVA_FAULT_INVALID_TASK_RESULT ? "INVALID_RESULT" :
                records[i].reason == NOVA_FAULT_TASK_RETURNED_RUNNING ? "BAD_RETURN_STATE" :
                records[i].reason == NOVA_FAULT_STACK_LIMIT ? "STACK_LIMIT" : "UNKNOWN";
            printf("  #%lu t=%lu ms task=%u %-12s %s\n",
                   (unsigned long)records[i].sequence, (unsigned long)records[i].tick_ms,
                   (unsigned)records[i].task_id, records[i].task_name, reason);
        }
    } else if (!strcmp(line, "devices")) {
        nova_device_t devices[NOVA_DEVICE_MAX];
        size_t count = nova_device_snapshot(devices, NOVA_DEVICE_MAX);
        printf("Devices: %lu/%u\n", (unsigned long)count, (unsigned)NOVA_DEVICE_MAX);
        for (size_t i = 0u; i < count; ++i)
            printf("  #%u %-12s %-8s caps=0x%02lx\n", (unsigned)devices[i].id,
                   devices[i].name, nova_device_class_name(devices[i].device_class),
                   (unsigned long)devices[i].capabilities);
    } else if (!strcmp(line, "device led0 on")) {
        const uint8_t value = 1u;
        nova_shell_write(nova_device_write("led0", &value, sizeof(value)) == 1 ? "led0 on\n" : "device write failed\n");
    } else if (!strcmp(line, "device led0 off")) {
        const uint8_t value = 0u;
        nova_shell_write(nova_device_write("led0", &value, sizeof(value)) == 1 ? "led0 off\n" : "device write failed\n");
    } else if (!strcmp(line, "crash")) {
        nova_crash_record_t record;
        if (nova_crash_read(&record)) {
            printf("Previous HardFault: PC=0x%08lx LR=0x%08lx xPSR=0x%08lx\n",
                   (unsigned long)record.pc, (unsigned long)record.lr,
                   (unsigned long)record.xpsr);
            printf("EXC_RETURN=0x%08lx frame=0x%08lx R0=0x%08lx R1=0x%08lx\n",
                   (unsigned long)record.exc_return, (unsigned long)record.frame_address,
                   (unsigned long)record.r0, (unsigned long)record.r1);
        } else {
            nova_shell_write("No retained HardFault record.\n");
        }
    } else if (!strcmp(line, "crash clear")) {
        nova_crash_clear();
        nova_shell_write("Retained crash record cleared.\n");
    } else if (!strcmp(line, "app status")) {
        nova_app_manager_info_t app;
        nova_app_manager_snapshot(&app);
        const char *state = app.state == NOVA_APP_STOPPED ? "STOPPED" :
            app.state == NOVA_APP_VALIDATING ? "VALIDATING" :
            app.state == NOVA_APP_READY ? "READY" :
            app.state == NOVA_APP_RUNNING ? "RUNNING" :
            app.state == NOVA_APP_STOPPING ? "STOPPING" : "FAILED";
        printf("Application: %s state=%s task=%u exit=%d\n", app.name, state,
               (unsigned)app.task_id, app.exit_code);
    } else if (!strcmp(line, "app stop")) {
        nova_shell_write(nova_app_manager_request_stop() ? "Application stop requested. The app must cooperate.\n" : "No active application can be stopped.\n");
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
        nova_usb_task();
        nova_kernel_poll();
        nova_app_manager_poll();
        nova_ui_poll();
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
