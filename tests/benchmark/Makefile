CKB-DEBUGGER := ckb-debugger
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH := $(ROOT_DIR)/../../build/ckb-js-vm

MAX-CYCLES ?= 2000000000
TEST-FILE ?=

define run
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r  2>&1 | fgrep 'Run result: 0'
endef

define debug
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r
endef

define compile-run
	$(CKB-DEBUGGER) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -c | awk -f $(ROOT_DIR)/../../tools/compile.awk | xxd -r -p > $(ROOT_DIR)/../../build/$(1).bc
	$(CKB-DEBUGGER) --read-file $(ROOT_DIR)/../../build/$(1).bc --bin $(BIN_PATH) -- -r | tee $(ROOT_DIR)/benchmark.txt
endef

all:
	$(call compile-run,benchmark.js)
