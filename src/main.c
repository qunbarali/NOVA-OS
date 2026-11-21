#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "nova/kernel.h"
#include "nova/board.h"
#include "nova/shell.h"
#include "nova/crash.h"
#include "nova/ui.h"
#include "nova/battery.h"
#include "nova/usb.h"
int main(void) {
    stdio_init_all();
    nova_usb_init();
    sleep_ms(1200);
    nova_kernel_init();
    nova_board_init();
    (void)nova_battery_init();
    nova_ui_init();
    multicore_launch_core1(nova_core1_entry);
    nova_shell_write("\n\nNOVA OS Kernel Alpha 1.5.0\n");
    nova_shell_write("Independent embedded OS research build\n");
    nova_shell_write("Type 'help' to list commands.\n\n");
    nova_shell_run();
    return 0;
}
