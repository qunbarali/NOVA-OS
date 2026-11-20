from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class NovaAlpha02Tests(unittest.TestCase):
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

    def test_firmware_workflow_is_manual_and_builds_both_targets(self):
        workflow = read(".github/workflows/firmware-build.yml")
        self.assertIn("workflow_dispatch:", workflow)
        self.assertNotIn("\n  push:", workflow)
        self.assertNotIn("\n  pull_request:", workflow)
        self.assertIn("board: pico", workflow)
        self.assertIn("board: pico2", workflow)
        self.assertIn("PICO_PLATFORM", workflow)
        self.assertIn("actions/upload-artifact@v4", workflow)

    def test_build_includes_kernel_services(self):
        cmake = read("CMakeLists.txt")
        self.assertIn("src/kernel/memory.c", cmake)
        self.assertIn("src/kernel/ipc.c", cmake)
        self.assertIn("-Wall", cmake)
        kernel = read("src/kernel/kernel.c")
        self.assertIn("add_repeating_timer_ms", kernel)
        self.assertIn("nova_system_ticks", kernel)


if __name__ == "__main__":
    unittest.main()
