# $Revision: 1589 $
# $Id: antlr.mk 1589 2007-03-21 09:44:36Z sjoyeux $

#################################################################
#       ANTLR grammar generator support for clbs
#     Copyright (c) 2005 LAAS/CNRS
#
#  Sylvain Joyeux <sylvain.joyeux@laas.fr>
#
# This generates files from ANTLR grammar files. It does not support (yet)
# more than one grammar in the same makefile.
#
# Building the grammar files
# --------------------------
# define the grammar name by 
#   GRAMMAR = name
# define the grammar source (.g file) with
#   name_SRC = grammar.g
# add optional options to antlr with
#   name_FLAGS = <options>
#  
# then include this file
#   include $(top_srcdir)/mk/antlr.mk
#
#
#
#
# Using generated files in other targets
# --------------------------------------
# Use the generated files in a regular target (app, lib). 
# For each grammar, the following variables are defined:
#   name_CC      list of generated .cc files 
#   name_HH      list of generated .hh files 
#   name_TXT     list of generated .txt files 
#
#
#
#
# Additional variables
# --------------------
#   name_PARSER  the parser name (if any)
#   name_LEXER   the lexer name (if any)
#   name_VOCAB   any exported vocabolary (by exportVocab)
#   name_OUTPUT  all output files
#       

$(GRAMMAR)_PARSER := $(shell grep 'class .*extends Parser' $(srcdir)/$($(GRAMMAR)_SRC) | awk '{ print $$2 }' | sed 's/;//')
$(GRAMMAR)_LEXER  := $(shell grep 'class .*extends Lexer' $(srcdir)/$($(GRAMMAR)_SRC) | awk '{ print $$2 }' | sed 's/;//')
$(GRAMMAR)_VOCAB  := $(shell grep 'exportVocab' $(srcdir)/$($(GRAMMAR)_SRC) | awk '{ print $$3 }' | sed 's/;//')

$(GRAMMAR)_CC=$(addsuffix .cc,$($(GRAMMAR)_PARSER) $($(GRAMMAR)_LEXER))
$(GRAMMAR)_HH=$(addsuffix .hh,$($(GRAMMAR)_PARSER) $($(GRAMMAR)_LEXER)) $(addsuffix TokenTypes.hh,$($(GRAMMAR)_LEXER) $($(GRAMMAR)_VOCAB))
$(GRAMMAR)_TXT=$(addsuffix TokenTypes.txt,$($(GRAMMAR)_LEXER) $($(GRAMMAR)_VOCAB))
$(GRAMMAR)_OUTPUT=$($(GRAMMAR)_CC) $($(GRAMMAR)_HH) $($(GRAMMAR)_TXT)

$($(GRAMMAR)_OUTPUT): $($(GRAMMAR)_SRC)
	@echo "Generating grammar $(GRAMMAR) (antlr)"
	@$(ANTLR) $($(GRAMMAR)_FLAGS) $<

	@for i in $($(GRAMMAR)_CC); do \
	    mv `echo $$i | sed 's/\.cc/.cpp/'` $$i; \
	    sed -i 's/\.hpp"/.hh"/' $$i; \
	done

	@for i in $($(GRAMMAR)_HH); do \
	    mv `echo $$i | sed 's/\.hh/.hpp/'` $$i; \
	    sed -i 's/\.hpp"/.hh"/' $$i; \
	done
		

.PRECIOUS: $($(GRAMMAR)_OUTPUT)

%.dep: %.g
	@echo "Making dependencies of $(notdir $<)"
	@echo "$($(GRAMMAR)_CC) $($(GRAMMAR)_HH) $($(GRAMMAR)_TXT): $<" >> $@

DEP_FILES += $($(GRAMMAR)_SRC:.g=.dep)

clean: $(GRAMMAR)-antlr-clean
$(GRAMMAR)-antlr-clean:
	@echo "Cleaning ANTLR output of $(GRAMMAR)"
	@rm -f $($(GRAMMAR)_OUTPUT)

.PHONY: $(GRAMMAR)-antlr-clean

