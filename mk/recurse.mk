# $Revision: 1022 $
# $Id: recurse.mk 1022 2005-10-11 12:06:01Z sjoyeux $

ifdef SUBDIRS 
build: recurse-build
clean: recurse-clean
distclean: recurse-distclean
install: recurse-install

recurse-%:
	@set -e ; for dir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$dir $*; \
	 done 
endif

