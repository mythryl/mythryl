FILES = various.c
H = LibH.libh
D = FFI
HF = ../libh.pkg
CF = various.make7

$(D)/$(CF): $(FILES)
	$(LIB7_BINDIR)/c-glue-maker $(CPPO) -include $(HF) -libhandle $(H) -dir $(D) -cmfile $(CF) $^
