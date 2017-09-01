define PINFO
PINFO DESCRIPTION=Graphics driver dll for TI OMAPL1xx Internal LCD Raster controller 
endef
EXTRA_CCDEPS += $(PROJECT_ROOT)/$(SECTION)/omapl1xx.h
CCFLAGS += -fno-toplevel-reorder # To disable the reordering of the top level functions, variables and asm statements
