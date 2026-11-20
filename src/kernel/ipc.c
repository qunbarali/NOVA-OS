#include "nova/ipc.h"
#include "nova/smp_lock.h"
#include <string.h>

static uint8_t take_waiter(uint8_t *waiters) {
    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((*waiters & bit) != 0u) {
            *waiters = (uint8_t)(*waiters & (uint8_t)~bit);
            return i;
        }
    }
    return NOVA_TASK_ID_INVALID;
}

static bool timeout_elapsed(uint32_t start, uint32_t timeout_ms) {
    return timeout_ms != 0u && (uint32_t)(nova_task_now_ms() - start) >= timeout_ms;
}

static uint32_t timeout_remaining(uint32_t start, uint32_t timeout_ms) {
    if (timeout_ms == 0u) return 0u;
    uint32_t elapsed = (uint32_t)(nova_task_now_ms() - start);
    return elapsed >= timeout_ms ? 0u : timeout_ms - elapsed;
}

void nova_queue_init(nova_queue_t *queue) {
    if (queue == NULL) return;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    memset(queue, 0, sizeof(*queue));
    nova_smp_lock_release(lock);
}

bool nova_queue_send(nova_queue_t *queue, const void *message, size_t length) {
    if (queue == NULL || message == NULL || length == 0u || length > NOVA_QUEUE_MESSAGE_SIZE) return false;
    nova_task_id_t wake = NOVA_TASK_ID_INVALID;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool sent = false;
    if (queue->count < NOVA_QUEUE_CAPACITY) {
        memset(queue->data[queue->tail], 0, NOVA_QUEUE_MESSAGE_SIZE);
        memcpy(queue->data[queue->tail], message, length);
        queue->lengths[queue->tail] = (uint8_t)length;
        queue->tail = (uint8_t)((queue->tail + 1u) % NOVA_QUEUE_CAPACITY);
        ++queue->count;
        wake = take_waiter(&queue->receive_waiters);
        sent = true;
    }
    nova_smp_lock_release(lock);
    if (wake != NOVA_TASK_ID_INVALID) (void)nova_task_wake(wake);
    return sent;
}

bool nova_queue_receive(nova_queue_t *queue, void *message, size_t capacity, size_t *received) {
    if (queue == NULL || message == NULL) return false;
    nova_task_id_t wake = NOVA_TASK_ID_INVALID;
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
            wake = take_waiter(&queue->send_waiters);
            result = true;
        }
    }
    nova_smp_lock_release(lock);
    if (wake != NOVA_TASK_ID_INVALID) (void)nova_task_wake(wake);
    return result;
}

bool nova_queue_send_wait(nova_queue_t *queue, const void *message, size_t length, uint32_t timeout_ms) {
    if (queue == NULL || message == NULL || length == 0u || length > NOVA_QUEUE_MESSAGE_SIZE) return false;
    nova_task_id_t caller = nova_task_current_id();
    if (caller == NOVA_TASK_ID_INVALID) return false;
    uint32_t start = nova_task_now_ms();
    uint8_t bit = (uint8_t)(1u << caller);
    for (;;) {
        nova_task_id_t wake = NOVA_TASK_ID_INVALID;
        nova_smp_lock_state_t lock = nova_smp_lock_acquire();
        queue->send_waiters = (uint8_t)(queue->send_waiters & (uint8_t)~bit);
        if (queue->count < NOVA_QUEUE_CAPACITY) {
            memcpy(queue->data[queue->tail], message, length);
            queue->lengths[queue->tail] = (uint8_t)length;
            queue->tail = (uint8_t)((queue->tail + 1u) % NOVA_QUEUE_CAPACITY);
            ++queue->count;
            wake = take_waiter(&queue->receive_waiters);
            nova_smp_lock_release(lock);
            if (wake != NOVA_TASK_ID_INVALID) (void)nova_task_wake(wake);
            return true;
        }
        if (timeout_elapsed(start, timeout_ms)) {
            nova_smp_lock_release(lock);
            return false;
        }
        queue->send_waiters |= bit;
        bool prepared = nova_task_prepare_block(timeout_remaining(start, timeout_ms));
        if (!prepared) queue->send_waiters = (uint8_t)(queue->send_waiters & (uint8_t)~bit);
        nova_smp_lock_release(lock);
        if (!prepared) return false;
        nova_task_commit_block();
    }
}

bool nova_queue_receive_wait(nova_queue_t *queue, void *message, size_t capacity, size_t *received, uint32_t timeout_ms) {
    if (queue == NULL || message == NULL) return false;
    nova_task_id_t caller = nova_task_current_id();
    if (caller == NOVA_TASK_ID_INVALID) return false;
    uint32_t start = nova_task_now_ms();
    uint8_t bit = (uint8_t)(1u << caller);
    for (;;) {
        nova_task_id_t wake = NOVA_TASK_ID_INVALID;
        nova_smp_lock_state_t lock = nova_smp_lock_acquire();
        queue->receive_waiters = (uint8_t)(queue->receive_waiters & (uint8_t)~bit);
        if (queue->count != 0u) {
            size_t length = queue->lengths[queue->head];
            if (capacity < length) {
                nova_smp_lock_release(lock);
                return false;
            }
            memcpy(message, queue->data[queue->head], length);
            if (received != NULL) *received = length;
            memset(queue->data[queue->head], 0, NOVA_QUEUE_MESSAGE_SIZE);
            queue->lengths[queue->head] = 0u;
            queue->head = (uint8_t)((queue->head + 1u) % NOVA_QUEUE_CAPACITY);
            --queue->count;
            wake = take_waiter(&queue->send_waiters);
            nova_smp_lock_release(lock);
            if (wake != NOVA_TASK_ID_INVALID) (void)nova_task_wake(wake);
            return true;
        }
        if (timeout_elapsed(start, timeout_ms)) {
            nova_smp_lock_release(lock);
            return false;
        }
        queue->receive_waiters |= bit;
        bool prepared = nova_task_prepare_block(timeout_remaining(start, timeout_ms));
        if (!prepared) queue->receive_waiters = (uint8_t)(queue->receive_waiters & (uint8_t)~bit);
        nova_smp_lock_release(lock);
        if (!prepared) return false;
        nova_task_commit_block();
    }
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
    if (acquired) { mutex->locked = true; mutex->owner = caller; }
    nova_smp_lock_release(lock);
    return acquired;
}

bool nova_mutex_unlock(nova_mutex_t *mutex) {
    if (mutex == NULL) return false;
    nova_task_id_t caller = nova_task_current_id();
    if (caller == NOVA_TASK_ID_INVALID) return false;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool released = mutex->locked && mutex->owner == caller;
    if (released) { mutex->locked = false; mutex->owner = NOVA_TASK_ID_INVALID; }
    nova_smp_lock_release(lock);
    return released;
}

void nova_event_init(nova_event_t *event) {
    if (event == NULL) return;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    event->flags = 0u;
    event->waiters = 0u;
    nova_smp_lock_release(lock);
}

void nova_event_set(nova_event_t *event, uint32_t flags) {
    if (event == NULL) return;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    event->flags |= flags;
    uint8_t waiters = event->waiters;
    event->waiters = 0u;
    nova_smp_lock_release(lock);
    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i)
        if ((waiters & (uint8_t)(1u << i)) != 0u) (void)nova_task_wake(i);
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

uint32_t nova_event_wait(nova_event_t *event, uint32_t mask, bool clear_on_exit, uint32_t timeout_ms) {
    if (event == NULL || mask == 0u) return 0u;
    nova_task_id_t caller = nova_task_current_id();
    if (caller == NOVA_TASK_ID_INVALID) return 0u;
    uint32_t start = nova_task_now_ms();
    uint8_t bit = (uint8_t)(1u << caller);
    for (;;) {
        nova_smp_lock_state_t lock = nova_smp_lock_acquire();
        event->waiters = (uint8_t)(event->waiters & (uint8_t)~bit);
        uint32_t matched = event->flags & mask;
        if (matched != 0u) {
            if (clear_on_exit) event->flags &= ~matched;
            nova_smp_lock_release(lock);
            return matched;
        }
        if (timeout_elapsed(start, timeout_ms)) {
            nova_smp_lock_release(lock);
            return 0u;
        }
        event->waiters |= bit;
        bool prepared = nova_task_prepare_block(timeout_remaining(start, timeout_ms));
        if (!prepared) event->waiters = (uint8_t)(event->waiters & (uint8_t)~bit);
        nova_smp_lock_release(lock);
        if (!prepared) return 0u;
        nova_task_commit_block();
    }
}
