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

CFLAGS := --target=riscv64 -march=rv64imc_zba_zbb_zbc_zbs
CFLAGS += -g -Oz \
		-Wall -Werror -Wno-nonnull -Wno-unused-function \
		-fno-builtin-printf -fno-builtin-memcmp \
		-nostdinc -nostdlib \
		-fdata-sections -ffunction-sections

CFLAGS += -I deps/ckb-c-stdlib/libc -I deps/ckb-c-stdlib
CFLAGS += -I include -I include/c-stdlib
CFLAGS += -I deps/compiler-rt-builtins-riscv/compiler-rt/lib/builtins

CFLAGS += -Wextra -Wno-sign-compare -Wno-missing-field-initializers -Wundef -Wuninitialized \
		  -Wunused -Wno-unused-parameter -Wchar-subscripts -funsigned-char -Wno-unused-function \
		  -DCONFIG_VERSION=\"2021-03-27-CKB\"
CFLAGS += -Wno-incompatible-library-redeclaration -Wno-implicit-const-int-float-conversion -Wno-invalid-noreturn

CFLAGS += -DCKB_DECLARATION_ONLY
CFLAGS += -D__BYTE_ORDER=1234 -D__LITTLE_ENDIAN=1234 -D__ISO_C_VISIBLE=1999 -D__GNU_VISIBLE
CFLAGS += -DCKB_MALLOC_DECLARATION_ONLY -DCKB_PRINTF_DECLARATION_ONLY -DCONFIG_BIGNUM -DCONFIG_STACK_CHECK
# uncomment to dump memory usage
# CFLAGS += -DMEMORY_USAGE

LDFLAGS := -static --gc-sections
LDFLAGS += -Ldeps/compiler-rt-builtins-riscv/build -lcompiler-rt

OBJDIR=build

QJS_OBJS=$(OBJDIR)/qjs.o $(OBJDIR)/quickjs.o $(OBJDIR)/libregexp.o $(OBJDIR)/libunicode.o \
		$(OBJDIR)/cutils.o $(OBJDIR)/mocked.o $(OBJDIR)/std_module.o $(OBJDIR)/ckb_module.o $(OBJDIR)/ckb_cell_fs.o \
		$(OBJDIR)/libbf.o $(OBJDIR)/cmdopt.o

STD_OBJS=$(OBJDIR)/string_impl.o $(OBJDIR)/malloc_impl.o $(OBJDIR)/math_impl.o \
		$(OBJDIR)/math_log_impl.o $(OBJDIR)/math_pow_impl.o $(OBJDIR)/printf_impl.o $(OBJDIR)/stdio_impl.o \
		$(OBJDIR)/locale_impl.o


all: build/ckb-js-vm

deps/compiler-rt-builtins-riscv/build/libcompiler-rt.a:
	cd deps/compiler-rt-builtins-riscv && make

build/ckb-js-vm: $(STD_OBJS) $(QJS_OBJS) $(OBJDIR)/impl.o deps/compiler-rt-builtins-riscv/build/libcompiler-rt.a
	$(LD) $(LDFLAGS) -o $@ $^
	cp $@ $@.debug
	$(OBJCOPY) --strip-debug --strip-all $@
	ls -lh build/ckb-js-vm

$(OBJDIR)/%.o: quickjs/%.c
	@echo build $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: include/c-stdlib/src/%.c
	@echo build $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: include/%.c
	@echo build $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/impl.o: deps/ckb-c-stdlib/libc/src/impl.c
	@echo build $<
	@$(CC) $(filter-out -DCKB_DECLARATION_ONLY, $(CFLAGS)) -c -o $@ $<

test:
	make -f tests/examples/Makefile
	make -f tests/basic/Makefile
	cd tests/ckb_js_tests && make all

benchmark:
	make -f tests/benchmark/Makefile

clean:
	rm -f build/*.o
	rm -f build/ckb-js-vm
	rm -f build/ckb-js-vm.debug
	cd tests/ckb_js_tests && make clean
	make -C deps/compiler-rt-builtins-riscv clean

STYLE := "{BasedOnStyle: Google, TabWidth: 4, IndentWidth: 4, UseTab: Never, SortIncludes: false, ColumnLimit: 120}"
fmt:
	clang-format-18 -i -style=$(STYLE) \
		quickjs/ckb_module.c \
		quickjs/ckb_module.h \
		quickjs/mocked.c \
		quickjs/mocked.h \
		quickjs/qjs.c \
		quickjs/std_module.c \
		quickjs/std_module.h

install:
	wget 'https://github.com/nervosnetwork/ckb-standalone-debugger/releases/download/v0.119.0/ckb-debugger-linux-x64.tar.gz'
	tar zxvf ckb-debugger-linux-x64.tar.gz
	mv ckb-debugger ~/.cargo/bin/ckb-debugger
	make -f tests/ckb_js_tests/Makefile install-lua

.phony: all clean
