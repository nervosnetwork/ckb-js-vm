#!/usr/bin/env bash
set -e
export NIX_HARDENING_ENABLE="all -zerocallusedregs"

# experimental support for reproducible build with nix
nix-shell --run "make clean && make all"
shasum -a 256 build/ckb-js-vm
