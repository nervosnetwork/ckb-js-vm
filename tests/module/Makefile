
CKB-DEBUGGER := ckb-debugger
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BIN_PATH := $(ROOT_DIR)/../../build/ckb-js-vm

MAX-CYCLES ?= 2000000000
TEST-FILE ?=

define run
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r
endef

define debug
	$(CKB-DEBUGGER) --max-cycles $(MAX-CYCLES) --read-file $(ROOT_DIR)/$(1) --bin $(BIN_PATH) -- -r
endef


all:
	$(call run,test_schnorr.js)
	$(call run,test_ckb.js)
	$(call run,test_secp256k1.js)
	$(call run,test_hash.js)
	$(call run,test_misc.js)
