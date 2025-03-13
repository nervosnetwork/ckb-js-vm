CKB_DEBUGGER ?= ckb-debugger
MAX_CYCLES ?= 2000000000
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm

define compile_js_to_bc
	$(CKB_DEBUGGER) --read-file $(1) --bin $(BIN_PATH) -- -c $(2)
endef

all: syscall \
	cell_target

syscall:
	cargo run --bin syscall | $(CKB_DEBUGGER) --tx-file=- -s lock

cell_target:
	cargo run --bin default_by_cell | $(CKB_DEBUGGER) -s lock --tx-file=- -- -t 91bc1d1fc8c19289e72d15004dd506b76246bfead13391a5b35179cf7c8f37ef01

clean:
	cargo clean
