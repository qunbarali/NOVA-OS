from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class NovaSourceContractTests(unittest.TestCase):
    def test_dual_board_build_script(self):
        script = read("scripts/build-all.ps1")
        self.assertIn("Board = 'pico'", script)
        self.assertIn("Board = 'pico2'", script)
        self.assertIn("PICO_SDK_PATH", script)

    def test_kernel_core1_diagnostics_are_atomic(self):
        kernel = read("src/kernel/kernel.c")
        self.assertIn("__atomic_add_fetch", kernel)
        self.assertIn("__atomic_load_n", kernel)
        self.assertIn("nova_core1_is_ready", kernel)

    def test_shell_has_bounded_input_and_reboot(self):
        shell = read("src/shell/shell.c")
        self.assertIn("LINE_CAPACITY", shell)
        self.assertIn("Input too long; command discarded", shell)
        self.assertIn('"reboot"', shell)

    def test_both_chip_memory_capacities_are_reported(self):
        board = read("src/hal/board.c")
        self.assertIn("return 264u", board)
        self.assertIn("return 520u", board)

    def test_build_has_warnings_and_uf2_outputs(self):
        cmake = read("CMakeLists.txt")
        self.assertIn("-Wall", cmake)
        self.assertIn("pico_add_extra_outputs(nova_os)", cmake)


if __name__ == "__main__":
    unittest.main()
