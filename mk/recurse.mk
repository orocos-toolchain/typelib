# $Revision: 1521 $
# $Id: recurse.mk 1521 2006-08-21 07:55:10Z sjoyeux $

ifdef SUBDIRS 
build: recurse-build
clean: recurse-clean
distclean: recurse-distclean
install: recurse-install
doc: recurse-doc

recurse-distclean: recurse-clean

recurse-%:
	@set -e ; for dir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$dir $*; \
	 done 
endif

