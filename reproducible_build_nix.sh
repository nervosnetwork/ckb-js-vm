#!/usr/bin/env bash
set -e
# experimental support for reproducible build with nix
nix-shell --pure --run "make clean && make all"
shasum -a 256 build/ckb-js-vm
