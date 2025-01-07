CKB_DEBUGGER ?= ckb-debugger
MAX_CYCLES ?= 2000000000
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm

all: \
	cargo_test \
	syscall \
	simple_udt \
	fs_mount

cargo_test:
	cargo test

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
