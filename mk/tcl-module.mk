# $Revision: 1265 $
# $Id: tcl-module.mk 1265 2006-01-18 15:10:54Z sjoyeux $

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
install: $(DESTDIR)$(tclmoduledir)/$(build_lib)
$$(DESTDIR)(tclmoduledir)/$(build_lib): $(build_lib)
	$(INSTALL_DIR) $(DESTDIR)$(tclmoduledir)
	$(INSTALL_LIB) $(build_lib) $(DESTDIR)$(tclmoduledir)

################ Cleanup
clean: $(TCLMODULE)-lib-clean
$(TCLMODULE)-lib-clean:
	-$(RM) $(build_lib) $($(TCLMODULE)_OBJS)

################ Dependencies
DEP_SRC += $($(TCLMODULE)_SRC)
DEP_CPPFLAGS += $($(TCLMODULE)_CPPFLAGS) $($(TCLMODULE)_CFLAGS) $(TCL_CPPFLAGS)

