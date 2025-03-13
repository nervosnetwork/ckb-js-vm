CKB_DEBUGGER ?= ckb-debugger
MAX_CYCLES ?= 2000000000
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm

BYTECODE_DIR = $(ROOT_DIR)/../../build/bytecode
JS_SOURCE_DIR = $(ROOT_DIR)/test_data/fs_module_mount
BYTECODE_FILES = index.bc fib_module.bc a/fib_module.bc b/fib_module.bc

define compile_js_to_bc
	$(CKB_DEBUGGER) --read-file $(1) --bin $(BIN_PATH) -- -c $(2)
endef

all: out \
	cargo_test \
	syscall \
	fs_bytecode \
	cell_target

out:
	@mkdir -p $(ROOT_DIR)/../../build/bytecode
	@mkdir -p $(ROOT_DIR)/../../build/bytecode/a
	@mkdir -p $(ROOT_DIR)/../../build/bytecode/b

cargo_test:
	cargo test

fs_bytecode:
	$(call compile_js_to_bc,$(JS_SOURCE_DIR)/index.js,$(BYTECODE_DIR)/index.bc)
	$(call compile_js_to_bc,$(JS_SOURCE_DIR)/fib_module.js,$(BYTECODE_DIR)/fib_module.bc)
	cp $(BYTECODE_DIR)/fib_module.bc $(BYTECODE_DIR)/a/fib_module.bc
	cp $(BYTECODE_DIR)/fib_module.bc $(BYTECODE_DIR)/b/fib_module.bc
	$(CKB_DEBUGGER) --max-cycles $(MAX_CYCLES) \
		--read-file $(BYTECODE_DIR)/fs_modules_bc.fs \
		--bin $(BIN_PATH) -- -f -r

syscall:
	cargo run --bin syscall | $(CKB_DEBUGGER) --tx-file=- -s lock

simple_udt:
	cargo run --bin simple_udt | $(CKB_DEBUGGER) --tx-file=- --script-group-type type --cell-type output --cell-index 0

secp256k1_blake160_lock:
	cargo run --bin secp256k1_blake160_lock | $(CKB_DEBUGGER) --tx-file=- --script-group-type lock --cell-type input --cell-index 0

cell_target:
	cargo run --bin default_by_cell | $(CKB_DEBUGGER) -s lock --tx-file=- -- -t 91bc1d1fc8c19289e72d15004dd506b76246bfead13391a5b35179cf7c8f37ef01

install-lua:
	sudo apt install lua5.4

clean:
	cargo clean
	rm -rf ../../build/bytecode
