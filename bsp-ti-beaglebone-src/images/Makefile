ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

HOST_MKIFS := mkifs

SUFFIXES := .build .ifs .raw

.PHONY: all clean

all: ifs-ti-beaglebone.bin 

clean: 
	$(RM_HOST) ifs-ti-beaglebone.bin 

ifs-ti-beaglebone.bin: beaglebone.build
	$(HOST_MKIFS) -r../install -v $(MKIFSFLAGS) $^ $@
