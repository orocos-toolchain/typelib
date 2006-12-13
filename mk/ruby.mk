# $Revision: 1535 $
# $Id: ruby.mk 1535 2006-10-11 14:07:32Z sjoyeux $

################ Build
build: $(MODULE)-build-rubyext
$(MODULE)_OBJS=	$(patsubst %.cc,%.lo,$($(MODULE)_SRC:%.c=%.lo))

CPPFLAGS := $(RUBY_CPPFLAGS) $(CPPFLAGS)
CFLAGS   := $(RUBY_CFLAGS) $(CFLAGS)
CXXFLAGS := $(RUBY_CFLAGS) $(CXXFLAGS)
################ Check if it is a convenience library or a plain one
build_lib = $(MODULE).la

install: $(DESTDIR)$(RUBY_EXTDIR)/$(build_lib)
$(DESTDIR)$(RUBY_EXTDIR)/$(build_lib): DESCRIPTION='Installing Ruby extension $(notdir $@) (libtool)'
$(DESTDIR)$(RUBY_EXTDIR)/$(build_lib): $(build_lib)
	$(INSTALL_DIR) $(DESTDIR)$(RUBY_EXTDIR)
	$(COMMAND_PREFIX)$(INSTALL_LIB) $(build_lib) $(DESTDIR)$(RUBY_EXTDIR)

install: $(addprefix $(DESTDIR)$(RUBY_EXTDIR)/,$($(MODULE)_HEADERS))
$(addprefix $(DESTDIR)$(RUBY_EXTDIR)/,$($(MODULE)_HEADERS)): DESCRIPTION='Installing extension headers in Ruby extension dir'
$(addprefix $(DESTDIR)$(RUBY_EXTDIR)/,$($(MODULE)_HEADERS)): $(DESTDIR)$(RUBY_EXTDIR)/%: $(srcdir)/%
	@$(INSTALL_DIR) $(DESTDIR)$(RUBY_EXTDIR)
	$(COMMAND_PREFIX)$(INSTALL_DATA) $(build_lib) $< $(DESTDIR)$(RUBY_EXTDIR)

install: $(addprefix $(DESTDIR)$(includedir)/,$($(MODULE)_HEADERS))
$(addprefix $(DESTDIR)$(includedir)/,$($(MODULE)_HEADERS)): DESCRIPTION='Installing extension headers in Typelib include dir'
$(addprefix $(DESTDIR)$(includedir)/,$($(MODULE)_HEADERS)): $(DESTDIR)$(includedir)/%: $(srcdir)/%
	@$(INSTALL_DIR) $(DESTDIR)$(includedir)
	$(COMMAND_PREFIX)$(INSTALL_DATA) $(build_lib) $< $(DESTDIR)$(includedir)

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

