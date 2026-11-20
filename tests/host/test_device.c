#include "nova/device.h"
#include "nova/smp_lock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int fake_read(void *context, void *buffer, size_t capacity) {
    if (context == NULL || buffer == NULL || capacity == 0u) return -1;
    ((unsigned char *)buffer)[0] = *(unsigned char *)context;
    return 1;
}
static int fake_write(void *context, const void *buffer, size_t length) {
    if (context == NULL || buffer == NULL || length == 0u) return -1;
    *(unsigned char *)context = ((const unsigned char *)buffer)[0];
    return 1;
}

int main(void) {
    unsigned char state = 0u;
    const nova_device_ops_t ops = { fake_read, fake_write, NULL };
    nova_device_t led = {
        .id = 0u, .name = "test-led", .device_class = NOVA_DEVICE_GPIO,
        .capabilities = NOVA_DEVICE_CAP_READ | NOVA_DEVICE_CAP_WRITE,
        .ops = &ops, .context = &state
    };
    nova_smp_lock_init();
    nova_device_manager_init();
    assert(nova_device_count() == 0u);
    assert(nova_device_register(&led) == 0);
    assert(nova_device_register(&led) == -1);
    assert(nova_device_count() == 1u);
    unsigned char value = 1u;
    assert(nova_device_write("test-led", &value, 1u) == 1);
    value = 0u;
    assert(nova_device_read("test-led", &value, 1u) == 1);
    assert(value == 1u);
    assert(nova_device_write("missing", &value, 1u) == -1);
    nova_device_t snapshot[NOVA_DEVICE_MAX];
    assert(nova_device_snapshot(snapshot, NOVA_DEVICE_MAX) == 1u);
    assert(strcmp(snapshot[0].name, "test-led") == 0);
    assert(strcmp(nova_device_class_name(NOVA_DEVICE_GPIO), "GPIO") == 0);
    puts("NOVA host device manager tests passed.");
    return 0;
}
