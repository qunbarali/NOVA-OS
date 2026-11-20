from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class NovaAlpha07Tests(unittest.TestCase):
    def test_dual_board_build_script(self):
        script = read("scripts/build-all.ps1")
        self.assertIn("Board = 'pico'", script)
        self.assertIn("Board = 'pico2'", script)
        self.assertIn("PICO_SDK_PATH", script)

    def test_scheduler_is_bounded_fair_and_cooperative(self):
        header = read("include/nova/task.h")
        source = read("src/kernel/task.c")
        self.assertIn("NOVA_TASK_MAX 8u", header)
        self.assertIn("NOVA_TASK_AGAIN", header)
        self.assertIn("nova_task_scheduler_poll", source)
        self.assertIn("NOVA_TASK_SLEEPING", source)
        self.assertIn("scan_start", source)
        self.assertIn("nova_task_current_id", source)

    def test_priority_fault_and_critical_section_foundations(self):
        header = read("include/nova/task.h")
        source = read("src/kernel/task.c")
        critical_h = read("include/nova/critical.h")
        critical_c = read("src/kernel/critical.c")
        self.assertIn("nova_task_set_priority", header)
        self.assertIn("NOVA_TASK_FAULTED", source)
        self.assertIn("scheduler_active", source)
        self.assertIn("nova_task_fault_count", header)
        self.assertIn("nova_task_fault_snapshot", header)
        self.assertIn("NOVA_FAULT_LOG_CAPACITY 8u", header)
        self.assertIn("record_task_fault", source)
        self.assertIn("NOVA_FAULT_STACK_GUARD", source)
        self.assertIn("NOVA_FAULT_INVALID_TASK_RESULT", source)
        self.assertIn("NOVA_FAULT_TASK_RETURNED_RUNNING", source)
        self.assertIn(chr(34) + "faults" + chr(34), shell)
        self.assertIn("nova_critical_enter", critical_h)
        self.assertIn("save_and_disable_interrupts", critical_c)
        self.assertIn("restore_interrupts", critical_c)

    def test_pool_has_fixed_capacity(self):
        header = read("include/nova/memory.h")
        source = read("src/kernel/memory.c")
        self.assertIn("NOVA_POOL_BLOCK_COUNT 8u", header)
        self.assertIn("nova_pool_alloc", source)
        self.assertIn("nova_pool_free", source)

    def test_ipc_is_bounded_and_preserves_message_length(self):
        header = read("include/nova/ipc.h")
        source = read("src/kernel/ipc.c")
        self.assertIn("NOVA_QUEUE_CAPACITY 8u", header)
        self.assertIn("NOVA_QUEUE_MESSAGE_SIZE 32u", header)
        self.assertIn("lengths[NOVA_QUEUE_CAPACITY]", header)
        self.assertIn("capacity < length", source)
        self.assertIn("mutex->owner != caller", source)

    def test_shell_polls_scheduler_and_reports_timer(self):
        shell = read("src/shell/shell.c")
        self.assertIn("nova_kernel_poll()", shell)
        self.assertIn('"task demo"', shell)
        self.assertIn('"task stop"', shell)
        self.assertIn("nova_system_ticks()", shell)
        self.assertIn("nova_kernel_timer_ready()", shell)
        self.assertIn("nova_task_fault_count()", shell)
        self.assertIn('NOVA_VERSION "0.7.0"', shell)
        self.assertIn("stack_used_bytes", shell)
        self.assertIn("(high-water)", shell)

    def test_firmware_workflow_is_manual_and_builds_both_targets(self):
        workflow = read(".github/workflows/firmware-build.yml")
        self.assertIn("workflow_dispatch:", workflow)
        self.assertNotIn("\n  push:", workflow)
        self.assertNotIn("\n  pull_request:", workflow)
        self.assertIn("board: pico", workflow)
        self.assertIn("board: pico2", workflow)
        self.assertIn("PICO_PLATFORM", workflow)
        self.assertIn("actions/upload-artifact@v4", workflow)

    def test_stack_watermark_scans_from_low_address_and_tracks_initial_frame(self):
        source = read("src/kernel/task.c")
        self.assertIn("task->stack[untouched] == NOVA_STACK_FILL_WORD", source)
        self.assertIn("NOVA_TASK_STACK_WORDS - untouched", source)
        self.assertIn("update_stack_usage(task);", source)
        host_test = read("tests/host/test_task_scheduler.c")
        self.assertIn("stack_used_bytes == 9u * sizeof(uint32_t)", host_test)

    def test_build_includes_kernel_services_and_both_context_ports(self):
        cmake = read("CMakeLists.txt")
        self.assertIn("src/kernel/memory.c", cmake)
        self.assertIn("src/kernel/ipc.c", cmake)
        self.assertIn("src/kernel/critical.c", cmake)
        self.assertIn("VERSION 0.7.0", cmake)
        self.assertIn("set(CMAKE_C_EXTENSIONS ON)", cmake)
        self.assertIn("src/arch/armv6m/call_on_stack.S", cmake)
        self.assertIn("src/arch/armv8m/call_on_stack.S", cmake)
        self.assertIn("NOVA_ARCH_ARMV6M=1", cmake)
        self.assertIn("NOVA_ARCH_ARMV8M=1", cmake)
        task_header = read("include/nova/task.h")
        task_source = read("src/kernel/task.c")
        self.assertIn("NOVA_TASK_STACK_SIZE 512u", task_header)
        self.assertIn("nova_arch_context_switch", task_source)
        self.assertIn("stack_guard_valid", task_source)
        self.assertIn("nova_task_yield", task_header)
        self.assertIn("task_bootstrap", task_source)
        self.assertIn("NOVA_INTERRUPT_STACK_WORDS", task_source)
        for path in ("src/arch/armv6m/call_on_stack.S", "src/arch/armv8m/call_on_stack.S"):
            port = read(path)
            self.assertIn("msr psp", port)
            self.assertIn("msr msp", port)
            self.assertIn("msr control", port)
        self.assertIn("-Wall", cmake)
        kernel = read("src/kernel/kernel.c")
        self.assertIn("add_repeating_timer_ms", kernel)
        self.assertIn("nova_system_ticks", kernel)


if __name__ == "__main__":
    unittest.main()
