#include "nova/ipc.h"
#include "nova/smp_lock.h"
#include <string.h>

void nova_queue_init(nova_queue_t *queue) {
    if (queue == NULL) return;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    memset(queue, 0, sizeof(*queue));
    nova_smp_lock_release(lock);
}

bool nova_queue_send(nova_queue_t *queue, const void *message, size_t length) {
    if (queue == NULL || message == NULL || length == 0u ||
        length > NOVA_QUEUE_MESSAGE_SIZE) return false;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool sent = false;
    if (queue->count < NOVA_QUEUE_CAPACITY) {
        memset(queue->data[queue->tail], 0, NOVA_QUEUE_MESSAGE_SIZE);
        memcpy(queue->data[queue->tail], message, length);
        queue->lengths[queue->tail] = (uint8_t)length;
        queue->tail = (uint8_t)((queue->tail + 1u) % NOVA_QUEUE_CAPACITY);
        ++queue->count;
        sent = true;
    }
    nova_smp_lock_release(lock);
    return sent;
}

bool nova_queue_receive(nova_queue_t *queue, void *message, size_t capacity, size_t *received) {
    if (queue == NULL || message == NULL) return false;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool result = false;
    if (queue->count != 0u) {
        size_t length = queue->lengths[queue->head];
        if (capacity >= length) {
            memcpy(message, queue->data[queue->head], length);
            if (received != NULL) *received = length;
            memset(queue->data[queue->head], 0, NOVA_QUEUE_MESSAGE_SIZE);
            queue->lengths[queue->head] = 0u;
            queue->head = (uint8_t)((queue->head + 1u) % NOVA_QUEUE_CAPACITY);
            --queue->count;
            result = true;
        }
    }
    nova_smp_lock_release(lock);
    return result;
}

size_t nova_queue_count(const nova_queue_t *queue) {
    if (queue == NULL) return 0u;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    size_t count = queue->count;
    nova_smp_lock_release(lock);
    return count;
}

void nova_mutex_init(nova_mutex_t *mutex) {
    if (mutex == NULL) return;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    mutex->locked = false;
    mutex->owner = NOVA_TASK_ID_INVALID;
    nova_smp_lock_release(lock);
}

bool nova_mutex_try_lock(nova_mutex_t *mutex) {
    if (mutex == NULL) return false;
    nova_task_id_t caller = nova_task_current_id();
    if (caller == NOVA_TASK_ID_INVALID) return false;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool acquired = !mutex->locked;
    if (acquired) {
        mutex->locked = true;
        mutex->owner = caller;
    }
    nova_smp_lock_release(lock);
    return acquired;
}

bool nova_mutex_unlock(nova_mutex_t *mutex) {
    if (mutex == NULL) return false;
    nova_task_id_t caller = nova_task_current_id();
    if (caller == NOVA_TASK_ID_INVALID) return false;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool released = mutex->locked && mutex->owner == caller;
    if (released) {
        mutex->locked = false;
        mutex->owner = NOVA_TASK_ID_INVALID;
    }
    nova_smp_lock_release(lock);
    return released;
}

void nova_event_init(nova_event_t *event) {
    if (event == NULL) return;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    event->flags = 0u;
    nova_smp_lock_release(lock);
}
void nova_event_set(nova_event_t *event, uint32_t flags) {
    if (event == NULL) return;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    event->flags |= flags;
    nova_smp_lock_release(lock);
}
uint32_t nova_event_get(const nova_event_t *event) {
    if (event == NULL) return 0u;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    uint32_t flags = event->flags;
    nova_smp_lock_release(lock);
    return flags;
}
uint32_t nova_event_clear(nova_event_t *event, uint32_t flags) {
    if (event == NULL) return 0u;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    uint32_t previous = event->flags;
    event->flags &= ~flags;
    nova_smp_lock_release(lock);
    return previous;
}
