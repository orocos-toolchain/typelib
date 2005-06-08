ANTLR=cantlr

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
	    mv $${i/cc/cpp} $$i; \
	    sed -i 's/\.hpp"/.hh"/' $$i; \
	done

	@for i in $($(GRAMMAR)_HH); do \
	    mv $${i/hh/hpp} $$i ; \
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

