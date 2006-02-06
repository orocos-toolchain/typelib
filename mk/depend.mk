# $Revision: 1055 $
# $Id: depend.mk 1055 2005-10-13 08:39:54Z sjoyeux $

DEP_FILES += $(patsubst %.c,%.dep,$(DEP_SRC:.cc=.dep))

### Include dependencies only if we aren't cleaning
ifeq ($(findstring clean,$(MAKECMDGOALS)),)
ifneq ($(DEP_FILES),)

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
endif

clean: dep-clean
dep-clean:
	@echo "Removing dependencies"
	@rm -f $(DEP_FILES)

