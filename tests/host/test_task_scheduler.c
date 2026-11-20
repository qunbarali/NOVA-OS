#include "nova/task.h"

#include <assert.h>
#include <stdio.h>

static unsigned task_a_runs;
static unsigned task_b_runs;
static unsigned task_exit_runs;
static unsigned task_fault_runs;
static unsigned execution_order[8];
static unsigned order_count;

static nova_task_result_t task_a(void *context) {
    (void)context;
    ++task_a_runs;
    if (task_a_runs == 1u) nova_task_sleep_current(10u);
    return NOVA_TASK_AGAIN;
}
static nova_task_result_t task_b(void *context) {
    (void)context;
    ++task_b_runs;
    return NOVA_TASK_AGAIN;
}
static nova_task_result_t task_exit(void *context) {
    (void)context;
    ++task_exit_runs;
    return NOVA_TASK_EXIT;
}
static nova_task_result_t task_fault(void *context) {
    (void)context;
    ++task_fault_runs;
    return (nova_task_result_t)99;
}
static nova_task_result_t priority_low(void *context) {
    execution_order[order_count++] = (unsigned)(uintptr_t)context;
    return NOVA_TASK_AGAIN;
}

int main(void) {
    nova_task_init();
    nova_task_id_t a = nova_task_create("task-a", task_a, NULL);
    nova_task_id_t b = nova_task_create("task-b", task_b, NULL);
    nova_task_id_t exiting = nova_task_create("task-exit", task_exit, NULL);
    assert(a != NOVA_TASK_ID_INVALID && b != NOVA_TASK_ID_INVALID);
    assert(exiting != NOVA_TASK_ID_INVALID && nova_task_count() == 3u);
    nova_task_info_t stack_info[NOVA_TASK_MAX];
    assert(nova_task_snapshot(stack_info, NOVA_TASK_MAX) == 3u);
    assert(stack_info[0].stack_size_bytes == NOVA_TASK_STACK_SIZE);

    nova_task_scheduler_poll(100u);
    assert(task_a_runs == 1u && task_b_runs == 1u && task_exit_runs == 1u);
    assert(nova_task_count() == 2u && nova_task_dispatch_count() == 3u);
    nova_task_scheduler_poll(109u);
    assert(task_a_runs == 1u && task_b_runs == 2u);
    nova_task_scheduler_poll(110u);
    assert(task_a_runs == 2u && task_b_runs == 3u);
    assert(nova_task_suspend(b));
    nova_task_scheduler_poll(111u);
    assert(task_b_runs == 3u);
    assert(nova_task_resume(b));
    nova_task_scheduler_poll(112u);
    assert(task_b_runs == 4u);
    assert(nova_task_terminate(a) && nova_task_terminate(b));
    assert(nova_task_count() == 0u);

    nova_task_init();
    task_a_runs = 0u;
    a = nova_task_create("wrap", task_a, NULL);
    assert(a != NOVA_TASK_ID_INVALID);
    nova_task_scheduler_poll(UINT32_MAX - 4u);
    assert(task_a_runs == 1u);
    nova_task_scheduler_poll(4u);
    assert(task_a_runs == 1u);
    nova_task_scheduler_poll(5u);
    assert(task_a_runs == 2u);

    nova_task_init();
    nova_task_id_t low = nova_task_create("low", priority_low, (void *)(uintptr_t)1u);
    nova_task_id_t high = nova_task_create("high", priority_low, (void *)(uintptr_t)2u);
    assert(low != NOVA_TASK_ID_INVALID && high != NOVA_TASK_ID_INVALID);
    assert(nova_task_set_priority(low, 200u));
    assert(nova_task_set_priority(high, 10u));
    nova_task_scheduler_poll(0u);
    assert(order_count == 2u && execution_order[0] == 2u && execution_order[1] == 1u);

    nova_task_init();
    nova_task_id_t bad = nova_task_create("bad", task_fault, NULL);
    assert(bad != NOVA_TASK_ID_INVALID);
    nova_task_scheduler_poll(0u);
    assert(task_fault_runs == 1u);
    assert(nova_task_fault_count() == 1u);
    nova_task_info_t snapshot[NOVA_TASK_MAX];
    assert(nova_task_snapshot(snapshot, NOVA_TASK_MAX) == 1u);
    assert(snapshot[0].state == NOVA_TASK_FAULTED);
    assert(nova_task_terminate(bad));
    assert(nova_task_count() == 0u);

    puts("NOVA host scheduler tests passed.");
    return 0;
}
