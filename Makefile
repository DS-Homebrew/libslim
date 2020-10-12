
export TOPDIR ?= $(CURDIR)
export LIBSLIM := $(TOPDIR)/libslim

OFILES := $(foreach dir,$(LIBSLIM)/native,$(notdir $(wildcard $(dir)/*.cpp)))

all: libslim examples

libslim: $(LIBSLIM)/lib/libslim.a
	cd libslim && $(MAKE)

examples: $(TOPDIR)/examples/simple/simple.nds
	cd examples && cd simple && $(MAKE)

$(TOPDIR)/examples/simple/simple.nds: $(TOPDIR)/examples/simple/source/directory.c

$(LIBSLIM)/lib/libslim.a: $(OFILES)

clean: 
	@echo clean ...
	@rm -fr $(LIBSLIM)/native
	@rm -fr $(LIBSLIM)/lib/libslim.a
	@rm -fr $(TOPDIR)/examples/simple/simple.nds
