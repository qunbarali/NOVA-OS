#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "nova/kernel.h"
#include "nova/board.h"
#include "nova/shell.h"
#include "nova/crash.h"
int main(void) {
    stdio_init_all();
    sleep_ms(1200);
    nova_board_init();
    nova_kernel_init();
    multicore_launch_core1(nova_core1_entry);
    nova_shell_write("\n\nNOVA OS Kernel Alpha 1.1.0\n");
    nova_shell_write("Independent embedded OS research build\n");
    nova_shell_write("Type 'help' to list commands.\n\n");
    nova_shell_run();
    return 0;
}
