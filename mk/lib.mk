# -*- Makefile -*-

LIB_NAME ?= $(PACKAGE)_$(MODULE)
build_lib = lib$(LIB_NAME).la

################ Build
build: $(MODULE)-build-lib

$(MODULE)-build-lib: $(builddir)/$(build_lib)

$(MODULE)_OBJS=	$(patsubst %.cc,%.lo,$($(MODULE)_SRC:%.c=%.lo))

$(build_lib): DESCRIPTION='Linking $@ (libtool)'
$(build_lib): $($(MODULE)_OBJS)
	$(COMMAND_PREFIX)$(LTLD) -o $@ -rpath $(libdir) -version-info $($(MODULE)_VERSION):$($(MODULE)_REVISION):$($(MODULE)_AGE) \
								$(LDFLAGS) $($(MODULE)_LDFLAGS) $(LIBS) $($(MODULE)_LIBS) $^


include $(top_srcdir)/mk/compile.mk

################ Install
install: $(libdir)/$(build_lib)
$(libdir)/$(build_lib): DESCRIPTION='Installing $(notdir $(build_lib)) (libtool)'
$(libdir)/$(build_lib): $(build_lib)
	$(INSTALL_DIR) $(libdir)
	$(COMMAND_PREFIX)$(INSTALL_LIB) $(build_lib) $(libdir)

################ Cleanup
clean: $(MODULE)-lib-clean
$(MODULE)-lib-clean: DESCRIPTION='Cleaning $(CURDIR) (libtool)'
$(MODULE)-lib-clean:
	-$(COMMAND_PREFIX)$(LTRM) $(build_lib) $($(MODULE)_OBJS)

################ Dependencies
DEP_SRC += $($(MODULE)_SRC)
DEP_CPPFLAGS += $($(MODULE)_CPPFLAGS)

