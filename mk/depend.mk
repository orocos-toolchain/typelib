# -*- Makefile -*-
# $Id: depend.mk,v 1.2 2005/01/12 12:16:48 fpy Exp $
#

DEP_FILES += $(patsubst %.c,%.dep,$(DEP_SRC:.cc=.dep))

### Include dependencies only if we aren't cleaning
ifeq ($(findstring clean,$(MAKECMDGOALS)),)

Makefile: dep-gen
dep-gen: $(DEP_FILES)

%.dep: %.cc
	@echo "Making dependencies of $(notdir $<)"
	@set -e ; $(CXX) $(DEP_FLAG) $(CPPFLAGS) $(DEP_CPPFLAGS) $< |\
	sed 's;\($*\)\.o *:;$(builddir)/\1.lo $(builddir)/\1.o $(builddir)/$(DEP_BASE)\1\.dep:;g' > $@  ;\
	[ -s $@ ] || rm -f $@

%.dep: %.c
	@echo "Making dependencies of $(notdir $<)"
	@set -e ; $(CC) $(DEP_FLAG) $(CPPFLAGS) $(DEP_CPPFLAGS) $< |\
	sed 's;\($*\)\.o *:;$(builddir)/\1.lo $(builddir)/\1.o $(builddir)/$(DEP_BASE)\1\.dep:;g' > $@  ;\
	[ -s $@ ] || rm -f $@

-include $(DEP_FILES)
endif

clean: dep-clean
dep-clean: DESCRIPTION='Cleaning $(CURDIR) (dependencies)'
dep-clean:
	-$(COMMAND_PREFIX)rm -f $(DEP_FILES)

