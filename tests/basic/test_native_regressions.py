"""Exercise native bindings and VM exit status through ckb-debugger."""

import os
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
DEBUGGER = os.environ.get("CKB_DEBUGGER", "ckb-debugger")


class NativeRegressions(unittest.TestCase):
    def run_script(self, source, success=True):
        result = subprocess.run(
            [DEBUGGER, "--max-cycles", "2000000000", "--bin",
             str(ROOT / "build/ckb-js-vm"), "--", "-e", source],
            capture_output=True, text=True, timeout=60,
        )
        output = result.stdout + result.stderr
        if success:
            self.assertEqual(result.returncode, 0, output)
            self.assertIn("Run result: 0", output)
        else:
            self.assertNotEqual(result.returncode, 0, output)
            self.assertRegex(output, r"Run result: -?\d+", output)
        return output

    def test_exec_argument_bounds(self):
        self.run_script('''
            import {execCell} from "@ckb-js-std/bindings";
            for (const n of [257, 300, 512]) {
                for (const hash of [new ArrayBuffer(32), null]) {
                    let caught = false;
                    try { execCell(hash, 0, 0, 0, ...Array(n).fill("x")); }
                    catch (e) { caught = e instanceof RangeError; }
                    if (!caught) throw new Error("missing argument limit");
                }
            }
            for (const n of [0, 256]) {
                try { execCell(new ArrayBuffer(32), 0, 0, 0, ...Array(n).fill("x")); }
                catch (e) {
                    if (e instanceof RangeError || typeof e.errorCode !== "number")
                        throw new Error("valid argument count rejected");
                }
            }
        ''')


if __name__ == "__main__":
    unittest.main()
