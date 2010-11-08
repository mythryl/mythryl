FILES = ll.h
H = LLHandle.ll
D = FFI
HF = ../llhandle.pkg
CF = ll.make7
CPPO =

$(D)/$(CF): $(FILES)
	$(LIB7_BINDIR)/c-glue-maker $(CPPO) -heavy -include $(HF) -libhandle $(H) -dir $(D) -cmfile $(CF) $^
