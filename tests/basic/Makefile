CKB-DEBUGGER := ckb-debugger
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm
MAX-CYCLES ?= 2000000000

define run
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r
endef

define debug
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r
endef

all: qjs-tests syntax-error log syscalls assert log

qjs-tests:
	$(call run,test_op_overloading.js)
	$(call run,test_loop.js)
	$(call run,test_language.js)
	$(call run,test_closure.js)
	$(call run,test_builtin.js)
	$(call run,test_bignum.js)
	$(call run,test_float.js)

exception:
	$(call debug,test_exception.js)

log:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "console.assert(scriptArgs[0] == 'hello'); console.assert(scriptArgs[1] == 'world');" hello world

assert:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "console.assert(true);"
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "console.assert(true); console.assert(false); " || echo "should fail"

syntax-error:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "ASDF ASDF" 2>&1 | grep "SyntaxError"

syscalls:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'import * as ckb from "@ckb-js-std/bindings"; ckb.exit(0);'
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'import * as ckb from "@ckb-js-std/bindings"; ckb.debug("hello, ckb");'
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'import * as ckb from "@ckb-js-std/bindings"; console.log(ckb.vmVersion(), ckb.currentCycles())'
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'import * as ckb from "@ckb-js-std/bindings"; console.log(ckb.SOURCE_INPUT, ckb.SOURCE_OUTPUT, ckb.CELL_FIELD_OCCUPIED_CAPACITY)'
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'import * as ckb from "@ckb-js-std/bindings"; console.log(100.1, ckb.SOURCE_GROUP_INPUT, ckb.SOURCE_GROUP_OUTPUT)'

