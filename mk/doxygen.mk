# $Revision: 1497 $
# $Id: doxygen.mk 1497 2006-06-27 07:56:13Z sjoyeux $

ifeq ($(HAS_DOXYGEN), yes)
doc: doxygen-doc
doxygen-doc:
	if [ -f Doxyfile ]; then \
	    $(DOXYGEN) Doxyfile; \
	elif [ -f "$(srcdir)/Doxyfile" ]; then \
	    $(DOXYGEN) "$(srcdir)/Doxyfile"; \
	fi
endif

