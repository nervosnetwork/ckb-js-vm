CC := clang-18
LD := ld.lld-18
OBJCOPY := llvm-objcopy-18
AR := llvm-ar-18
RANLIB := llvm-ranlib-18

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	CC := clang
	LD := ld.lld
	AR := llvm-ar
	OBJCOPY := llvm-objcopy
	RANLIB := llvm-ranlib
endif

CFLAGS_TARGET = --target=riscv64 -march=rv64imc_zba_zbb_zbc_zbs
CFLAGS_OPTIMIZE = -g -Oz -fdata-sections -ffunction-sections
CFLAGS_WARNNING = -Wno-incompatible-library-redeclaration -Wno-invalid-noreturn -Wno-implicit-const-int-float-conversion
CFLAGS_NO_BUILTIN = -fno-builtin-printf -fno-builtin-memcmp
CFLAGS_DEFINE = -D__BYTE_ORDER=1234 \
	-D__LITTLE_ENDIAN=1234 \
	-D__ISO_C_VISIBLE=1999 \
	-D__GNU_VISIBLE \
	-DCKB_MALLOC_DECLARATION_ONLY \
	-DCKB_PRINTF_DECLARATION_ONLY
CFLAGS_BASE = $(CFLAGS_TARGET) $(CFLAGS_OPTIMIZE) $(CFLAGS_WARNNING) $(CFLAGS_NO_BUILTIN) $(CFLAGS_DEFINE)
CFLAGS_BASE_CKB_C_STDLIB = $(CFLAGS_BASE) \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib
CFLAGS_BASE_LIBC = $(CFLAGS_BASE) \
	-I libc \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-DCKB_DECLARATION_ONLY
CFLAGS_BASE_SRC = $(CFLAGS_BASE) \
	-I libc \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-I deps/quickjs \
	-I deps/secp256k1/include \
	-I deps/secp256k1/src \
	-DCKB_DECLARATION_ONLY \
	-DCONFIG_BIGNUM
CFLAGS_BASE_QUICKJS = $(CFLAGS_BASE) \
	-I libc \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-DCKB_DECLARATION_ONLY \
	-DCONFIG_BIGNUM \
	-DEMSCRIPTEN \
	-DCONFIG_STACK_CHECK \
	-DCONFIG_VERSION=\"2024-01-13-CKB\"
CFLAGS_BASE_SECP256k1 = $(CFLAGS_BASE) \
	-I libc \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-I deps/secp256k1/src \
	-I deps/secp256k1/include \
	-DCKB_DECLARATION_ONLY \
	-DECMULT_WINDOW_SIZE=6 \
	-DENABLE_MODULE_RECOVERY \
	-DENABLE_MODULE_SCHNORRSIG \
	-DENABLE_MODULE_EXTRAKEYS

LDFLAGS := -static --gc-sections
LDFLAGS += -Ldeps/compiler-rt-builtins-riscv/build -lcompiler-rt

all: out build/ckb-js-vm

out:
	@mkdir -p build
	@mkdir -p build/bytecode
	@mkdir -p build/ckb-c-stdlib
	@mkdir -p build/libc
	@mkdir -p build/src
	@mkdir -p build/quickjs
	@mkdir -p build/secp256k1

deps/compiler-rt-builtins-riscv/build/libcompiler-rt.a:
	cd deps/compiler-rt-builtins-riscv && make -j $(nproc)

build/ckb-js-vm: build/ckb-c-stdlib/impl.o \
                 build/libc/ckb_cell_fs.o \
                 build/libc/ctype.o \
                 build/libc/fenv.o \
                 build/libc/locale.o \
                 build/libc/malloc.o \
                 build/libc/math.o \
                 build/libc/math_log.o \
                 build/libc/math_pow.o \
                 build/libc/printf.o \
                 build/libc/stdio.o \
                 build/libc/stdlib.o \
                 build/libc/string.o \
                 build/libc/sys_time.o \
                 build/libc/time.o \
                 build/quickjs/quickjs.o \
                 build/quickjs/libregexp.o \
                 build/quickjs/libunicode.o \
                 build/quickjs/cutils.o \
                 build/quickjs/libbf.o \
                 build/secp256k1/secp256k1.o \
                 build/secp256k1/precomputed_ecmult.o \
                 build/src/ckb_module.o \
                 build/src/secp256k1_module.o \
				 build/src/hash_module.o \
				 build/src/misc_module.o \
                 build/src/qjs.o \
                 build/src/std_module.o \
				 build/src/utils.o \
                 deps/compiler-rt-builtins-riscv/build/libcompiler-rt.a
	$(LD) $(LDFLAGS) -o $@ $^
	cp $@ $@.debug
	$(OBJCOPY) --strip-debug --strip-all $@
	ls -lh build/ckb-js-vm

build/ckb-c-stdlib/%.o: deps/ckb-c-stdlib/libc/src/%.c
	@echo build $<
	@$(CC) $(CFLAGS_BASE_CKB_C_STDLIB) -c -o $@ $<

build/libc/%.o: libc/src/%.c
	@echo build $<
	@$(CC) $(CFLAGS_BASE_LIBC) -c -o $@ $<

build/src/%.o: src/%.c
	@echo build $<
	@$(CC) $(CFLAGS_BASE_SRC) -c -o $@ $<

build/quickjs/%.o: deps/quickjs/%.c
	@echo build $<
	@$(CC) $(CFLAGS_BASE_QUICKJS) -c -o $@ $<

test:
	make -f tests/examples/Makefile
	make -f tests/basic/Makefile
	make -f tests/module/Makefile

benchmark:
	make -f tests/benchmark/Makefile

# secp256k1
build/secp256k1/secp256k1.o: deps/secp256k1/src/secp256k1.c
	@echo build $<
	$(CC) $(CFLAGS_BASE_SECP256k1) -c -o $@ $<

build/secp256k1/precomputed_ecmult.o: deps/secp256k1/src/precomputed_ecmult.c
	@echo build $<
	$(CC) $(CFLAGS_BASE_SECP256k1) -c -o $@ $<

clean:
	rm -rf build
	make -C deps/compiler-rt-builtins-riscv clean

STYLE := "{BasedOnStyle: Google, TabWidth: 4, IndentWidth: 4, UseTab: Never, SortIncludes: false, ColumnLimit: 120}"
fmt:
	clang-format-18 -i -style=$(STYLE) \
		libc/*.h \
		libc/internal/*.h \
		libc/src/*.c \
		libc/sys/*.h \
		src/*

checksum: all
	shasum -a 256 build/ckb-js-vm > checksums.txt

install:
	npm install -g pnpm

.phony: all clean
