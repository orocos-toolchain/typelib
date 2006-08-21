# $Revision: 1521 $
# $Id: autotools-root.mk 1521 2006-08-21 07:55:10Z sjoyeux $

distclean: autotools-root-distclean
autotools-root-distclean: clean
	rm -f config.status libtool config.log $(CONFIG_OUTPUT_FILES) mk/init.mk Init.make

clean: pch-clean

include $(top_srcdir)/mk/autotools.mk

ifeq ($(HAS_TEST_SUPPORT), yes)
SUBDIRS += test
test: std-test
std-test:
	[ -f $(top_builddir)/test/Makefile ] && $(MAKE) -C $(top_builddir)/test test
endif

