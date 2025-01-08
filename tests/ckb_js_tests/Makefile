CKB_DEBUGGER ?= ckb-debugger
MAX_CYCLES ?= 2000000000
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm
FS_PACKER = node $(ROOT_DIR)/../../tools/fs-packer/dist/index.js

BYTECODE_DIR = ../../build/bytecode
JS_SOURCE_DIR = test_data/fs_module_mount
BYTECODE_FILES = $(BYTECODE_DIR)/index.bc $(BYTECODE_DIR)/fib_module.bc

%.bc: $(JS_SOURCE_DIR)/%.js
	$(CKB_DEBUGGER) --read-file $< --bin $(BIN_PATH) -- -c | \
		awk -f $(ROOT_DIR)/../../tools/compile.awk | \
		xxd -r -p > $@

all: \
	cargo_test \
	syscall \
	simple_udt \
	fs_mount \
	fs_bytecode \
	cell_target

cargo_test:
	cargo test

fs_bytecode: $(BYTECODE_FILES)
	cd $(BYTECODE_DIR) && $(FS_PACKER) pack fs_modules_bc.fs $(notdir $(BYTECODE_FILES))
	$(CKB_DEBUGGER) --max-cycles $(MAX_CYCLES) \
		--read-file $(BYTECODE_DIR)/fs_modules_bc.fs \
		--bin $(BIN_PATH) -- -f -r 2>&1 | \
		fgrep 'Run result: 0'

syscall:
	cargo run --bin syscall | $(CKB_DEBUGGER) --tx-file=- -s lock

fs_mount:
	cd test_data/fs_module_mount && $(FS_PACKER) pack ../../../../build/bytecode/fib_module.fs fib_module.js
	cd test_data/fs_module_mount && $(FS_PACKER) pack ../../../../build/bytecode/fib_module_mount.fs index.js init.js
	cargo run --bin module_mount | ${CKB_DEBUGGER} --tx-file=- -s lock

simple_udt:
	cargo run --bin simple_udt | $(CKB_DEBUGGER) --tx-file=- --script-group-type type --cell-type output --cell-index 0

cell_target:
	cargo run --bin default_by_cell | $(CKB_DEBUGGER) -s lock --tx-file=- -- -t 91bc1d1fc8c19289e72d15004dd506b76246bfead13391a5b35179cf7c8f37ef01

install-lua:
	sudo apt install lua5.4

clean:
	cargo clean
	rm -rf ../../build/bytecode
