# $Revision: 1381 $
# $Id: lib.mk 1381 2006-02-07 13:46:29Z sjoyeux $

################ Build
build: $(MODULE)-build-rubyext
$(MODULE)_OBJS=	$(patsubst %.cc,%.lo,$($(MODULE)_SRC:%.c=%.lo))

$(MODULE)_CPPFLAGS += $(RUBY_CPPFLAGS)
$(MODULE)_CFLAGS += $(RUBY_CFLAGS)
$(MODULE)_CXXFLAGS += $(RUBY_CFLAGS)

################ Check if it is a convenience library or a plain one
build_lib = $(MODULE).la

install: $(DESTDIR)$(RUBY_EXTDIR)/$(build_lib)
$(DESTDIR)$(RUBY_EXTDIR)/$(build_lib): DESCRIPTION='Installing Ruby extension $(notdir $@) (libtool)'
$(DESTDIR)$(RUBY_EXTDIR)/$(build_lib): $(build_lib)
	$(INSTALL_DIR) $(DESTDIR)$(RUBY_EXTDIR)
	$(COMMAND_PREFIX)$(INSTALL_LIB) $(build_lib) $(DESTDIR)$(RUBY_EXTDIR)

$(MODULE)_LIB_DEPENDS=$(filter %.la,$($(MODULE)_LIBS))
ifneq ($($(MODULE)_LIB_DEPENDS),)
$($(MODULE)_LIB_DEPENDS): | recurse-build
endif
recurse-build:

################ Link targets
$(MODULE)-build-rubyext: $(builddir)/$(build_lib)
$(build_lib): DESCRIPTION='Linking Ruby extension $(notdir $@) (libtool)'
$(build_lib): $($(MODULE)_OBJS) $($(MODULE)_LIB_DEPENDS)
	$(COMMAND_PREFIX)$(LTLD) -module -avoid-version -rpath $(RUBY_EXTDIR) -o $@ \
                $(RUBY_LDFLAGS) $(LDFLAGS) $($(MODULE)_LDFLAGS) $($(MODULE)_LIBS) $(LIBS) \
                $($(MODULE)_OBJS)

include $(top_srcdir)/mk/compile.mk

################ Cleanup
clean: $(MODULE)-lib-clean
$(MODULE)-lib-clean: DESCRIPTION='Cleaning Ruby extension $(notdir $@) (libtool)'
$(MODULE)-lib-clean:
	-$(COMMAND_PREFIX)$(LTRM) $(build_lib) $($(MODULE)_OBJS)

################ Dependencies
DEP_SRC += $($(MODULE)_SRC)
DEP_CPPFLAGS += $($(MODULE)_CPPFLAGS)

ifneq ($(SUBDIRS),)
$(MODULE)-lib-clean: recurse-clean
$(MODULE)-lib-install: recurse-install
$(MODULE)-lib-distclean: recurse-distclean
$(MODULE)-lib-build: recurse-build
endif

