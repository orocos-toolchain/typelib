# $Revision: 1495 $
# $Id: recurse.mk 1495 2006-06-27 07:54:04Z sjoyeux $

ifdef SUBDIRS 
build: recurse-build
clean: recurse-clean
distclean: recurse-distclean
install: recurse-install
doc: recurse-doc

recurse-%:
	@set -e ; for dir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$dir $*; \
	 done 
endif

