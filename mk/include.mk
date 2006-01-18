# $Revision: 1265 $
# $Id: include.mk 1265 2006-01-18 15:10:54Z sjoyeux $

build:
clean:

echo:
	@echo $(HEADERS);

SRC_HEADERS := $(shell cd $(srcdir) && find . -type f \( -name '*.h' -o -name '*.hh' -o -name '*.tcc' \))
INSTALLED_SRC_HEADERS=$(addprefix $(DESTDIR)$(pkgincludedir)/,$(SRC_HEADERS))
$(INSTALLED_SRC_HEADERS): $(DESTDIR)$(pkgincludedir)/%: $(srcdir)/%
	@$(INSTALL_DIR) -d $(dir $@)
	$(INSTALL_HEADER) $< $@

ifneq ($(srcdir),$(builddir))
BUILD_HEADERS := $(shell cd $(builddir) && find . -type f \( -name '*.h' -o -name '*.hh' -o -name '*.tcc' \) )
INSTALLED_BUILD_HEADERS=$(addprefix $(DESTDIR)$(pkgincludedir)/,$(BUILD_HEADERS))
$(INSTALLED_BUILD_HEADERS): $(DESTDIR)$(pkgincludedir)/%: $(builddir)/%
	@$(INSTALL_DIR) -d $(dir $@)
	$(INSTALL_HEADER) $< $@
endif
	
install: $(INSTALLED_SRC_HEADERS) $(INSTALLED_BUILD_HEADERS)
	
