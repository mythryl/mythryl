FILES = math.h
H = LibH.libh
D = FFI
HF = ../libh.pkg
CF = math.make7

$(D)/$(CF): $(FILES)
	$(LIB7_BINDIR)/c-glue-maker -include $(HF) -libhandle $(H) -dir $(D) -cmfile $(CF) $^
