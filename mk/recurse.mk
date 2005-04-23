# -*- Makefile -*-
# $Id: recurse.mk,v 1.1 2005/01/09 14:46:28 fpy Exp $

ifdef SUBDIRS 
recurse-%:
	@for dir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$dir $*; \
	 done 
endif
