# -*- Makefile -*-
# $Id: Init.make.in,v 1.2 2005/02/17 14:53:53 fpy Exp $

# include $(top_builddir)/Init.make

distclean: autotools-distclean
autotools-distclean::
	rm -f Makefile

#relative_dir=$(subst $(abs_top_builddir)/,,$(CURDIR)/)
#AC_FILES= Init.make mk/init.mk $(relative_dir)/Makefile
#$(addprefix $(top_builddir)/,$(AC_FILES)): $(top_builddir)/config.status
#$(addprefix $(top_builddir)/,$(AC_FILES)): $(top_builddir)/%: $(top_srcdir)/%.in
#	cd $(top_builddir) && ./config.status $(subst $(top_builddir)/,,$@)
#
#$(top_builddir)/config.status: $(top_srcdir)/configure
#	cd $(top_builddir) && ./config.status --recheck && ./config.status
#
#$(top_srcdir)/configure: $(top_srcdir)/configure.ac $(top_srcdir)/aclocal.m4 $(top_srcdir)/acinclude.m4
#	cd $(top_srcdir) && ./autogen.sh


