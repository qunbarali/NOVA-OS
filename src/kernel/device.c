#include "nova/device.h"
#include <stdbool.h>
#include "nova/smp_lock.h"
#include <string.h>

static nova_device_t devices[NOVA_DEVICE_MAX];
static size_t device_count_value;

void nova_device_manager_init(void) {
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    memset(devices, 0, sizeof(devices));
    device_count_value = 0u;
    nova_smp_lock_release(lock);
}

int nova_device_register(const nova_device_t *device) {
    if (device == NULL || device->name[0] == '\0' || device->ops == NULL ||
        device->device_class < NOVA_DEVICE_GPIO || device->device_class > NOVA_DEVICE_SENSOR ||
        (device->capabilities & ~(NOVA_DEVICE_CAP_READ | NOVA_DEVICE_CAP_WRITE | NOVA_DEVICE_CAP_CTRL)) != 0u)
        return -1;
    size_t length = 0u;
    while (length <= NOVA_DEVICE_NAME_MAX && device->name[length] != 0) ++length;
    if (length == 0u || length > NOVA_DEVICE_NAME_MAX) return -1;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    if (device_count_value >= NOVA_DEVICE_MAX) {
        nova_smp_lock_release(lock);
        return -1;
    }
    for (size_t i = 0u; i < device_count_value; ++i) {
        if (strcmp(devices[i].name, device->name) == 0) {
            nova_smp_lock_release(lock);
            return -1;
        }
    }
    nova_device_t *slot = &devices[device_count_value];
    *slot = *device;
    slot->id = (uint8_t)device_count_value;
    slot->name[NOVA_DEVICE_NAME_MAX] = '\0';
    int id = (int)device_count_value;
    ++device_count_value;
    nova_smp_lock_release(lock);
    return id;
}

size_t nova_device_count(void) {
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    size_t count = device_count_value;
    nova_smp_lock_release(lock);
    return count;
}

size_t nova_device_snapshot(nova_device_t *out, size_t capacity) {
    if (out == NULL || capacity == 0u) return 0u;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    size_t count = device_count_value < capacity ? device_count_value : capacity;
    memcpy(out, devices, count * sizeof(*out));
    nova_smp_lock_release(lock);
    return count;
}

static bool get_device(const char *name, nova_device_t *out) {
    if (name == NULL || out == NULL) return false;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool found = false;
    for (size_t i = 0u; i < device_count_value; ++i) {
        if (strcmp(devices[i].name, name) == 0) {
            *out = devices[i];
            found = true;
            break;
        }
    }
    nova_smp_lock_release(lock);
    return found;
}

int nova_device_read(const char *name, void *buffer, size_t capacity) {
    nova_device_t device;
    if (buffer == NULL || !get_device(name, &device) ||
        (device.capabilities & NOVA_DEVICE_CAP_READ) == 0u || device.ops->read == NULL) return -1;
    return device.ops->read(device.context, buffer, capacity);
}

int nova_device_write(const char *name, const void *buffer, size_t length) {
    nova_device_t device;
    if (buffer == NULL || !get_device(name, &device) ||
        (device.capabilities & NOVA_DEVICE_CAP_WRITE) == 0u || device.ops->write == NULL) return -1;
    return device.ops->write(device.context, buffer, length);
}

int nova_device_control(const char *name, uint32_t command, void *argument) {
    nova_device_t device;
    if (!get_device(name, &device) ||
        (device.capabilities & NOVA_DEVICE_CAP_CTRL) == 0u || device.ops->control == NULL) return -1;
    return device.ops->control(device.context, command, argument);
}

const char *nova_device_class_name(nova_device_class_t device_class) {
    switch (device_class) {
        case NOVA_DEVICE_GPIO: return "GPIO";
        case NOVA_DEVICE_CONSOLE: return "CONSOLE";
        case NOVA_DEVICE_STORAGE: return "STORAGE";
        case NOVA_DEVICE_DISPLAY: return "DISPLAY";
        case NOVA_DEVICE_USB: return "USB";
        case NOVA_DEVICE_SENSOR: return "SENSOR";
        default: return "UNKNOWN";
    }
}
