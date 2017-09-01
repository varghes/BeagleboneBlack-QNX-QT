vpath %.idl	$(VPATH.idl)

MAKEFILE = idl.make

IDLFLAGS += --codegen-qcm --relative-paths
IDLFLAGS.skel += --qcm-skel
IDLFLAGS.impl += --qcm-impl
IDLFLAGS.common += 

IDLINCLUDES = $(INCLUDES.idl)

IDL = idl

IDL.skel := $(IDL) $(IDLFLAGS) $(IDLFLAGS.skel) $(IDLINCLUDES)
IDL.impl := $(IDL) $(IDLFLAGS) $(IDLFLAGS.impl) $(IDLINCLUDES)
IDL.common := $(IDL) $(IDLFLAGS) $(IDLFLAGS.common) $(IDLINCLUDES)

%.h: %.idl
	$(IDL.common) $< 

%.c: %.idl
	$(IDL.common) $< 

%_impl.c: %.idl
	$(IDL.impl) $<

%_impl.h: %.idl
	$(IDL.impl) --no-poa-ties --no-poa-stubs $<
	@-rm $*_impl.c

%_skel.c: %.idl
	$(IDL.skel) $<
