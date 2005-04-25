distclean:
	rm -f config.status libtool config.log $(CONFIG_OUTPUT_FILES) 

include $(top_srcdir)/mk/autotools.mk

