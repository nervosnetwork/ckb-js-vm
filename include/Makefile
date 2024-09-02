STYLE := "{BasedOnStyle: Google, TabWidth: 4, IndentWidth: 4, UseTab: Never, SortIncludes: false, ColumnLimit: 120}"

fmt:
	clang-format-18 -i -style=$(STYLE) *.c *.h
	clang-format-18 -i -style=$(STYLE) c-stdlib/*.h
	clang-format-18 -i -style=$(STYLE) c-stdlib/src/*.c
	clang-format-18 -i -style=$(STYLE) ../quickjs/qjs.c ../quickjs/std_module.c ../quickjs/std_module.h ../quickjs/ckb_module.c ../quickjs/ckb_module.h
