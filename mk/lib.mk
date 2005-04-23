# -*- Makefile -*-
# $Id: lib.mk,v 1.1 2005/01/09 14:46:28 fpy Exp $

LIB_NAME ?= $(PACKAGE)_$(MODULE)
build_lib = lib$(LIB_NAME).la

################ Build
build: $(LIB_NAME)-build-lib

$(LIB_NAME)-build-lib: $(builddir)/$(build_lib)

$(LIB_NAME)_OBJS=	$(patsubst %.cc,%.lo,$($(MODULE)_SRC:%.c=%.lo))

$(build_lib): $($(LIB_NAME)_OBJS)
	$(LTLD) -o $@ -rpath $(libdir) -version-info $($(MODULE)_VERSION):$($(MODULE)_REVISION):$($(MODULE)_AGE) \
								$(LDFLAGS) $($(MODULE)_LDFLAGS) $(LIBS) $($(MODULE)_LIBS) $^

%.lo: %.c
	$(LTCXX) $(CFLAGS) $($(MODULE)_CXXFLAGS) -c $(CPPFLAGS) $($(MODULE)_CPPFLAGS) -o $@ $<

%.lo: %.cc
	$(LTCXX) $(CXXFLAGS) $($(MODULE)_CXXFLAGS) -c $(CPPFLAGS) $($(MODULE)_CPPFLAGS) -o $@ $<

################ Install
install: $(libdir)/$(build_lib)
$(libdir)/$(build_lib): $(build_lib)
	$(INSTALL_DIR) $(libdir)
	$(INSTALL_LIB) $(build_lib) $(libdir)

################ Cleanup
clean: $(LIB_NAME)-lib-clean
$(LIB_NAME)-lib-clean:
	-$(LTRM) $(build_lib) $($(LIB_NAME)_OBJS)

################ Dependencies
DEP_SRC += $($(MODULE)_SRC)
DEP_CPPFLAGS += $($(MODULE)_CPPFLAGS)

