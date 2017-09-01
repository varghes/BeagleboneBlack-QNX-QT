ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

include ../../../prodroot_pkt.mk
iopkt_root=../../../../..

EXTRA_INCVPATH+= $(IOPKT_ROOT)/sys $(IOPKT_ROOT)/sys/sys-nto $(IOPKT_ROOT)/lib/socket/public

LIBS = drvrS cacheS

NAME = devnp-$(PROJECT)

USEFILE=$(PROJECT_ROOT)/$(NAME).use

define PINFO
PINFO DESCRIPTION=am335x ethernet driver
endef

include $(MKFILES_ROOT)/qtargets.mk

INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../../install
USE_INSTALL_ROOT=1
