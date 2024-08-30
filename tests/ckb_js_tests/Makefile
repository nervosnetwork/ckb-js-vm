CKB_DEBUGGER ?= ckb-debugger
MAX_CYCLES ?= 2000000000
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm

all: \
	cargo_test \
	file_system \
	syscall \
	fs_bytecode\
	simple_udt\
	fs_mount

cargo_test:
	cargo test

spawn_caller:
	clang-18 --target=riscv64 -march=rv64imc_zba_zbb_zbc_zbs \
		-I ../../deps/ckb-c-stdlib/libc -I ../../deps/ckb-c-stdlib \
		-nostdinc -nostdlib -o ../../build/spawn_caller test_data/spawn_caller.c

module: spawn_caller
	cargo run --bin module | ${CKB_DEBUGGER} --tx-file=- -s lock

build/testdata_fs_modules.bin: test_data/fs_module/main.js test_data/fs_module/fib_module.js
	cd test_data/fs_module && lua ../../../../tools/fs.lua pack ../../../../$@ main.js fib_module.js

fs_bytecode:
	mkdir -p ../../build/bytecode
	$(CKB_DEBUGGER) --read-file test_data/fs_module/main.js --bin $(BIN_PATH) -- -c | lua $(ROOT_DIR)/../../tools/compile.lua | xxd -r -p > ../../build/bytecode/main.bc
	$(CKB_DEBUGGER) --read-file test_data/fs_module/fib_module.js --bin $(BIN_PATH) -- -c | lua $(ROOT_DIR)/../../tools/compile.lua | xxd -r -p > ../../build/bytecode/fib_module.bc
	cd ../../build/bytecode && lua ../../tools/fs.lua pack ../../build/testdata_fs_modules_bc.bin main.bc fib_module.bc
	$(CKB_DEBUGGER) --max-cycles $(MAX_CYCLES) --read-file ../../build/testdata_fs_modules_bc.bin --bin $(BIN_PATH) -- -f -r 2>&1 | fgrep 'Run result: 0'

file_system: build/testdata_fs_modules.bin
	cargo run --bin default_by_cell | $(CKB_DEBUGGER) -s lock --tx-file=- --read-file ../../$^ -- -f -r  2>&1 | fgrep 'Run result: 0'

syscall:
	cargo run --bin syscall | $(CKB_DEBUGGER) --tx-file=- -s lock

fs_mount:
	cd test_data/fs_module_mount && lua ../../../../tools/fs.lua pack ../../../../build/fib_module.bin fib_module.js
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
