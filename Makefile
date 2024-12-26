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
CFLAGS_BASE = $(CFLAGS_TARGET) $(CFLAGS_OPTIMIZE) $(CFLAGS_WARNNING)
CFLAGS_BASE_CKB_C_STDLIB = $(CFLAGS_BASE) \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-DCKB_MALLOC_DECLARATION_ONLY \
	-DCKB_PRINTF_DECLARATION_ONLY
CFLAGS_BASE_LIBC = $(CFLAGS_BASE) \
	-I libc \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-DCKB_MALLOC_DECLARATION_ONLY \
	-DCKB_PRINTF_DECLARATION_ONLY
CFLAGS_BASE_NNCP = $(CFLAGS_BASE) \
	-I libc \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-DCKB_MALLOC_DECLARATION_ONLY \
	-DCKB_PRINTF_DECLARATION_ONLY \
	-DCKB_DECLARATION_ONLY
CFLAGS_BASE_SRC = $(CFLAGS_BASE) \
	-I libc \
	-I deps/nncp \
	-I deps/quickjs \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-DCKB_MALLOC_DECLARATION_ONLY \
	-DCKB_PRINTF_DECLARATION_ONLY \
	-DCKB_DECLARATION_ONLY \
	-DCONFIG_BIGNUM \
	-fno-builtin-printf
CFLAGS_BASE_QUICKJS = $(CFLAGS_BASE) \
	-I libc \
	-I deps/ckb-c-stdlib/libc \
	-I deps/ckb-c-stdlib \
	-DCKB_MALLOC_DECLARATION_ONLY \
	-DCKB_PRINTF_DECLARATION_ONLY \
	-DCKB_DECLARATION_ONLY \
	-DCONFIG_BIGNUM -DEMSCRIPTEN \
	-DCONFIG_VERSION=\"2021-03-27-CKB\"

LDFLAGS := -static --gc-sections
LDFLAGS += -Ldeps/compiler-rt-builtins-riscv/build -lcompiler-rt

all: out build/ckb-js-vm

out:
	mkdir -p build
	mkdir -p build/bytecode
	mkdir -p build/ckb-c-stdlib
	mkdir -p build/libc
	mkdir -p build/nncp
	mkdir -p build/src
	mkdir -p build/quickjs

deps/compiler-rt-builtins-riscv/build/libcompiler-rt.a:
	cd deps/compiler-rt-builtins-riscv && make

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
                 build/nncp/cmdopt.o \
                 build/quickjs/quickjs.o \
                 build/quickjs/libregexp.o \
                 build/quickjs/libunicode.o \
                 build/quickjs/cutils.o \
                 build/quickjs/libbf.o \
                 build/quickjs/repl.o \
                 build/quickjs/qjscalc.o \
                 build/src/ckb_module.o \
                 build/src/qjs.o \
                 build/src/std_module.o \
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
	@$(CC) $(CFLAGS_BASE_LIBC) -DCKB_DECLARATION_ONLY -c -o $@ $<

build/nncp/%.o: deps/nncp/%.c
	@echo build $<
	@$(CC) $(CFLAGS_BASE_NNCP) -c -o $@ $<

build/src/%.o: src/%.c
	@echo build $<
	@$(CC) $(CFLAGS_BASE_SRC) -c -o $@ $<

build/quickjs/%.o: deps/quickjs/%.c
	@echo build $<
	@$(CC) $(CFLAGS_BASE_QUICKJS) -c -o $@ $<

test:
	make -f tests/examples/Makefile
	make -f tests/basic/Makefile
	cd tests/ckb_js_tests && make all

benchmark:
	make -f tests/benchmark/Makefile

clean:
	rm -rf build
	cd tests/ckb_js_tests && make clean
	make -C deps/compiler-rt-builtins-riscv clean

STYLE := "{BasedOnStyle: Google, TabWidth: 4, IndentWidth: 4, UseTab: Never, SortIncludes: false, ColumnLimit: 120}"
fmt:
	clang-format-18 -i -style=$(STYLE) \
		libc/*.h \
		libc/internal/*.h \
		libc/src/*.c \
		libc/sys/*.h \
		src/*

install:
	wget 'https://github.com/nervosnetwork/ckb-standalone-debugger/releases/download/v0.119.0/ckb-debugger-linux-x64.tar.gz'
	tar zxvf ckb-debugger-linux-x64.tar.gz
	mv ckb-debugger ~/.cargo/bin/ckb-debugger
	make -f tests/ckb_js_tests/Makefile install-lua

.phony: all clean
