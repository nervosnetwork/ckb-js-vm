CKB_DEBUGGER ?= ckb-debugger
MAX_CYCLES ?= 2000000000
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm

all: \
	cargo_test \
	file_system \
	syscall \
	fs_bytecode \
	simple_udt \
	fs_mount \
	module

cargo_test:
	cargo test

spawn_caller:
	clang-18 --target=riscv64 -march=rv64imc_zba_zbb_zbc_zbs \
		-I ../../deps/ckb-c-stdlib/libc -I ../../deps/ckb-c-stdlib \
		-nostdinc -nostdlib -o ../../build/bytecode/spawn_caller test_data/spawn_caller.c

module: spawn_caller
	cargo run --bin module | ${CKB_DEBUGGER} --tx-file=- -s lock

build/bytecode/fs_modules.fs: test_data/fs_module/index.js test_data/fs_module/fib_module.js
	cd test_data/fs_module && node ../../../../tools/fs-packer/dist/index.js pack ../../../../$@ index.js fib_module.js

fs_bytecode:
	$(CKB_DEBUGGER) --read-file test_data/fs_module/index.js --bin $(BIN_PATH) -- -c | awk -f $(ROOT_DIR)/../../tools/compile.awk | xxd -r -p > ../../build/bytecode/index.bc
	$(CKB_DEBUGGER) --read-file test_data/fs_module/fib_module.js --bin $(BIN_PATH) -- -c | awk -f $(ROOT_DIR)/../../tools/compile.awk | xxd -r -p > ../../build/bytecode/fib_module.bc
	cd ../../build/bytecode && node ../../tools/fs-packer/dist/index.js pack ../../build/bytecode/fs_modules_bc.fs index.bc fib_module.bc
	$(CKB_DEBUGGER) --max-cycles $(MAX_CYCLES) --read-file ../../build/bytecode/fs_modules_bc.fs --bin $(BIN_PATH) -- -f -r 2>&1 | fgrep 'Run result: 0'

file_system: build/bytecode/fs_modules.fs
	cargo run --bin default_by_cell | $(CKB_DEBUGGER) -s lock --tx-file=- --read-file ../../$^ -- -f -r  2>&1 | fgrep 'Run result: 0'

syscall:
	cargo run --bin syscall | $(CKB_DEBUGGER) --tx-file=- -s lock

fs_mount:
	cd test_data/fs_module_mount && node ../../../../tools/fs-packer/dist/index.js pack ../../../../build/bytecode/fib_module.fs fib_module.js
	cd test_data/fs_module_mount && node ../../../../tools/fs-packer/dist/index.js pack ../../../../build/bytecode/fib_module_mount.fs index.js init.js

	cargo run --bin module_mount | ${CKB_DEBUGGER} --tx-file=- -s lock

simple_udt:
	cargo run --bin simple_udt | $(CKB_DEBUGGER) --tx-file=- --script-group-type type --cell-type output --cell-index 0

cell_target:
	echo "blake2b(../../basic/test_loop.js) == 91bc1d1fc8c19289e72d15004dd506b76246bfead13391a5b35179cf7c8f37ef"
	cargo run --bin default_by_cell | $(CKB_DEBUGGER) -s lock --tx-file=- -- -t 91bc1d1fc8c19289e72d15004dd506b76246bfead13391a5b35179cf7c8f37ef01

install-lua:
	sudo apt install lua5.4

clean:
	cargo clean
	rm -rf ../../build/testdata_fs_modules.bin
