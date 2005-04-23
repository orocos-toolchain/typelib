# -*- Makefile -*-

LIB_NAME ?= $(PACKAGE)_$(MODULE)
build_lib = lib$(LIB_NAME).la

################ Build
build: $(MODULE)-build-lib

$(MODULE)-build-lib: $(builddir)/$(build_lib)

$(MODULE)_OBJS=	$(patsubst %.cc,%.lo,$($(MODULE)_SRC:%.c=%.lo))

$(build_lib): $($(MODULE)_OBJS)
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
clean: $(MODULE)-lib-clean
$(MODULE)-lib-clean:
	-$(LTRM) $(build_lib) $($(MODULE)_OBJS)

################ Dependencies
DEP_SRC += $($(MODULE)_SRC)
DEP_CPPFLAGS += $($(MODULE)_CPPFLAGS)

