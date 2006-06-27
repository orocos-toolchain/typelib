ifeq ($(HAS_DOXYGEN), yes)
doc: doxygen-doc
doxygen-doc:
	if [ -f Doxyfile ]; then \
	    $(DOXYGEN) Doxyfile; \
	elif [ -f "$(srcdir)/Doxyfile" ]; then \
	    $(DOXYGEN) "$(srcdir)/Doxyfile"; \
	fi
endif

