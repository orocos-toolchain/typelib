# $Revision: 1022 $
# $Id: include.mk 1022 2005-10-11 12:06:01Z sjoyeux $

build:
clean:

echo:
	@echo $(HEADERS);

SRC_HEADERS := $(shell cd $(srcdir) && find . -type f \( -name '*.h' -o -name '*.hh' -o -name '*.tcc' \))
INSTALLED_SRC_HEADERS=$(addprefix $(pkgincludedir)/,$(SRC_HEADERS))
$(INSTALLED_SRC_HEADERS): $(pkgincludedir)/%: $(srcdir)/%
	@$(INSTALL_DIR) -d $(dir $@)
	$(INSTALL_HEADER) $< $@

ifneq ($(srcdir),$(builddir))
BUILD_HEADERS := $(shell cd $(builddir) && find . -type f \( -name '*.h' -o -name '*.hh' -o -name '*.tcc' \) )
INSTALLED_BUILD_HEADERS=$(addprefix $(pkgincludedir)/,$(BUILD_HEADERS))
$(INSTALLED_BUILD_HEADERS): $(pkgincludedir)/%: $(builddir)/%
	@$(INSTALL_DIR) -d $(dir $@)
	$(INSTALL_HEADER) $< $@
endif
	
install: $(INSTALLED_SRC_HEADERS) $(INSTALLED_BUILD_HEADERS)
	
