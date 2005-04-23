distclean:
	rm -f config.status libtool $(CONFIG_FILES) config.log

include $(top_srcdir)/mk/autotools.mk

