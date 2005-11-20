# $Revision: 1052 $
# $Id: autotools-root.mk 1052 2005-10-13 07:46:02Z sjoyeux $

distclean: autotools-root-distclean
autotools-root-distclean:
	rm -f config.status libtool config.log $(CONFIG_OUTPUT_FILES) 

clean: pch-clean

include $(top_srcdir)/mk/autotools.mk

