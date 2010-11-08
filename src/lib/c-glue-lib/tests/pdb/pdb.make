FILES = pdb.c forward.c
H = PDBHandle.pdb
D = FFI
HF = ../pdbhandle.pkg
CF = pdb.make7
CPPO = -D__builtin_va_list=int

$(D)/$(CF): $(FILES)
	$(LIB7_BINDIR)/c-glue-maker $(CPPO) -heavy -include $(HF) -libhandle $(H) -dir $(D) -cmfile $(CF) $^
