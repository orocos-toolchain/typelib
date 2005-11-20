# $Revision: 1022 $
# $Id: tcl-module.mk 1022 2005-10-11 12:06:01Z sjoyeux $

build_lib=$(TCLMODULE)$(TCL_SHLIB_SUFFIX)

################ Build
build: $(TCLMODULE)-build-lib

$(TCLMODULE)-build-lib: $(builddir)/$(build_lib)

$(TCLMODULE)_OBJS=	$(patsubst %.cc,%.o,$($(TCLMODULE)_SRC:%.c=%.o))

$(build_lib): $($(TCLMODULE)_OBJS)
	$(TCL_SHLIB_LD) $(TCL_SHLIB_LDFLAGS) -o $@ \
		$(LDFLAGS) $($(TCLMODULE)_LDFLAGS) $(LIBS) $($(TCLMODULE)_LIBS) $^

%.o: %.c
	$(TCL_CC) $(TCL_SHLIB_CFLAGS) $(TCL_CPPFLAGS) $(TCL_EXTRA_CFLAGS) \
		$(CFLAGS) $($(TCLMODULE)_CFLAGS) -c $(CPPFLAGS) $($(TCLMODULE)_CPPFLAGS) -o $@ $<

################ Install
install: $(tclmoduledir)/$(build_lib)
$(tclmoduledir)/$(build_lib): $(build_lib)
	$(INSTALL_DIR) $(tclmoduledir)
	$(INSTALL_LIB) $(build_lib) $(tclmoduledir)

################ Cleanup
clean: $(TCLMODULE)-lib-clean
$(TCLMODULE)-lib-clean:
	-$(RM) $(build_lib) $($(TCLMODULE)_OBJS)

################ Dependencies
DEP_SRC += $($(TCLMODULE)_SRC)
DEP_CPPFLAGS += $($(TCLMODULE)_CPPFLAGS) $($(TCLMODULE)_CFLAGS) $(TCL_CPPFLAGS)

