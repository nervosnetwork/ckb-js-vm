
CKB-DEBUGGER := ckb-debugger
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH = $(ROOT_DIR)/../../build/ckb-js-vm
MAX-CYCLES ?= 2000000000

define run
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r  2>&1 | fgrep 'Run result: 0'
endef

define debug
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r
endef

all: qjs-tests syntax-error log syscalls assert

qjs-tests:
	$(call run,test_op_overloading.js)
	$(call run,test_loop.js)
	$(call run,test_language.js)
	$(call run,test_closure.js)
	$(call run,test_builtin.js)
	$(call run,test_bignum.js)

log:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "console.log(scriptArgs[0], scriptArgs[1]);" hello world


assert:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "console.assert(true);"
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "console.assert(true); console.assert(false); " || echo "should fail"

syntax-error:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "ASDF" | grep "ReferenceError: 'ASDF' is not defined"

syscalls:
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "console.log(ckb);"
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e "ckb.exit(0);"
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'ckb.debug("hello, ckb");'
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'console.log(ckb.vm_version(), ckb.current_cycles())'
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'console.log(ckb.SOURCE_INPUT, ckb.SOURCE_OUTPUT, ckb.CELL_FIELD_OCCUPIED_CAPACITY)'
	$(CKB-DEBUGGER) --bin $(BIN_PATH) -- -e 'console.log(100.1, ckb.SOURCE_GROUP_INPUT, ckb.SOURCE_GROUP_OUTPUT)'
