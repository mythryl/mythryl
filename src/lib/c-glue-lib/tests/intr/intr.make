FILES = intr.c
H = LibH.libh
D = FFI
HF = ../libh.pkg
CF = intr.make7
CPPO = -D__builtin_va_list=int

$(D)/$(CF): $(FILES)
	$(LIB7_BINDIR)/c-glue-maker $(CPPO) -include $(HF) -libhandle $(H) -dir $(D) -cmfile $(CF) $^
