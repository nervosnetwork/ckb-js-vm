"""Exercise native bindings and VM exit status through ckb-debugger."""

import os
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
DEBUGGER = os.environ.get("CKB_DEBUGGER", "ckb-debugger")


class NativeRegressions(unittest.TestCase):
    def test_filesystem_survives_queued_imports(self):
        source = '''
            Promise.resolve().then(() => {
                globalThis.buffers = Array.from({length: 64},
                    () => new Uint8Array(1024).fill(0x58));
                return import('./late.js');
            }).then(m => {
                if (m.value !== 42) throw new Error('wrong module');
                console.log('late import OK');
            });
        '''
        files = [('index.js', source), ('padding', ' ' * 65536),
                 ('late.js', 'export const value = 42;')]
        entries = bytearray()
        data = bytearray()
        for name, content in files:
            name, content = name.encode() + b'\0', content.encode()
            entries += struct.pack('<IIII', len(data), len(name),
                                   len(data) + len(name), len(content))
            data += name + content + b'\0'
        archive = struct.pack('<I', len(files)) + entries + data
        vm = (ROOT / 'build/ckb-js-vm').read_bytes()

        def ckb_hash(data):
            return '0x' + hashlib.blake2b(
                data, digest_size=32, person=b'ckb-default-hash').hexdigest()

        def out_point(index):
            return {'tx_hash': '0x' + format(index, '064x'), 'index': '0x0'}

        script = {'code_hash': ckb_hash(vm), 'hash_type': 'data2',
                  'args': '0x0100' + ckb_hash(archive)[2:] + '04'}
        output = {'capacity': '0x2540be400', 'lock': script, 'type': None}
        deps = [{'out_point': out_point(i), 'dep_type': 'code'} for i in [1, 2]]
        tx_input = {'since': '0x0', 'previous_output': out_point(3)}
        tx = {
            'mock_info': {
                'inputs': [{'input': tx_input, 'output': output, 'data': '0x'}],
                'cell_deps': [{'cell_dep': dep, 'output': output, 'data': '0x' + buf.hex()}
                              for dep, buf in zip(deps, [vm, archive])],
                'header_deps': [], 'extensions': [],
            },
            'tx': {'version': '0x0', 'cell_deps': deps, 'header_deps': [],
                   'inputs': [tx_input], 'outputs': [], 'outputs_data': [], 'witnesses': []},
        }
        with tempfile.TemporaryDirectory() as directory:
            tx_path = Path(directory) / 'tx.json'
            tx_path.write_text(json.dumps(tx))
            fs_path = Path(directory) / 'archive.fs'
            fs_path.write_bytes(archive)
            for args in [
                ['--tx-file', str(tx_path)],
                ['--tx-file', str(tx_path), '--', '-t', ckb_hash(archive)[2:] + '04', '-f'],
                ['--read-file', str(fs_path), '--bin', str(ROOT / 'build/ckb-js-vm'),
                 '--', '-r', '-f'],
            ]:
                result = subprocess.run([DEBUGGER, *args], capture_output=True,
                                        text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn('late import OK', result.stdout)

    def test_async_failures_reject_transaction(self):
        for source in [
            'await Promise.resolve(); throw new Error("async failure");',
            'Promise.reject(new Error("async failure"));',
            'Promise.resolve().then(() => { throw new Error("async failure"); });',
            '(async () => { await 0; throw new Error("async failure"); })();',
            'Promise.reject(null);',
            'Promise.reject(undefined);',
            '''const p = Promise.reject({toString() {
                   p.catch(() => {}); return "late handler";
               }});''',
            'await new Promise(() => {});',
            '''import {evalJsScript} from "@ckb-js-std/bindings";
               globalThis.reason = new Error("shared reason");
               Promise.reject(reason);
               try { evalJsScript("throw globalThis.reason", true); } catch (_) {}''',
        ]:
            with self.subTest(source=source):
                self.run_script(source, success=False)

    def test_handled_async_failures_succeed(self):
        self.run_script('''
            import {evalJsScript} from "@ckb-js-std/bindings";
            await Promise.resolve();
            await Promise.reject("expected").catch(() => {});
            const p = Promise.reject("handled in a later job");
            Promise.resolve().then(() => p.catch(() => {}));
            for (const module of [false, true]) {
                let caught = false;
                try { evalJsScript('throw new Error("expected")', module); }
                catch (e) { caught = e.message === "expected"; }
                if (!caught) throw new Error("lost exception");
            }
        ''')

    def test_long_module_names(self):
        for length in [252, 253, 512, 2048]:
            output = self.run_script('import "' + 'a' * length + '.js";', success=False)
            self.assertIn("ReferenceError", output)
            self.assertNotIn("memory error", output)

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
