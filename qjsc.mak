# Compiler settings
CC = clang-18
CFLAGS += -Ideps/quickjs
CFLAGS += -Wall -Wno-array-bounds -Wno-format-truncation -Wno-implicit-const-int-float-conversion -fwrapv
CFLAGS += -D_GNU_SOURCE -DCONFIG_VERSION=\"2024-01-13\" -DCONFIG_BIGNUM
CFLAGS += -O2
CFLAGS += -DCONFIG_LTO -flto

STYLE := "{BasedOnStyle: Google, TabWidth: 4, IndentWidth: 4, UseTab: Never, SortIncludes: false, ColumnLimit: 120}"

# Output directory
OBJDIR = build/qjsc

# Object files
OBJS = $(OBJDIR)/qjsc.o \
       $(OBJDIR)/quickjs.o \
       $(OBJDIR)/libregexp.o \
       $(OBJDIR)/libunicode.o \
       $(OBJDIR)/cutils.o \
       $(OBJDIR)/quickjs-libc.o \
       $(OBJDIR)/libbf.o

# Linker flags
LDFLAGS = -flto
LIBS = -lm

# Main target
build/qjsc/qjsc: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJDIR)/qjsc.o: tools/qjsc/qjsc.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: deps/quickjs/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Include dependency files
-include $(OBJS:.o=.o.d)

fmt:
	clang-format-18 -i -style=$(STYLE) tools/qjsc/qjsc.c

# Clean target
clean:
	rm -rf $(OBJDIR)

.PHONY: clean fmt
