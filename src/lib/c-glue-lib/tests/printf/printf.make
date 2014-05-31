FFI/printf.make7: printf.h
	$(LIB7_BINDIR)/c-glue-maker -include ../libh.pkg -libhandle LibH.libh -dir FFI -cmfile printf.make7 printf.h
