# 
# Copyright (C) 1996-2005 LAAS/CNRS 
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#    - Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    - Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 
# $LAAS: Makefile.in,v 1.10 2004/09/24 13:55:53 matthieu Exp $

build_lib=$(TCLMODULE)$(TCL_SHLIB_SUFFIX)

################ Build
build: $(TCLMODULE)-build-lib

$(TCLMODULE)-build-lib: $(builddir)/$(build_lib)

$(TCLMODULE)_OBJS=	$(patsubst %.cc,%.o,$($(TCLMODULE)_SRC:%.c=%.o))

$(build_lib): $($(TCLMODULE)_OBJS)
	$(TCL_SHLIB_LD) $(TCL_SHLIB_LDFLAGS) -o $@ \
		$(LDFLAGS) $($(TCLMODULE)_LDFLAGS) $(LIBS) $($(TCLMODULE)_LIBS) $^

%.o: %.c
	$(TCL_CC) $(TCL_SHLIB_CFLAGS) $(TCL_CPPFLAGS) $(TCL_EXTRA_CFLAGS) \
		$(CFLAGS) $($(TCLMODULE)_CFLAGS) -c $(CPPFLAGS) $($(TCLMODULE)_CPPFLAGS) -o $@ $<

################ Install
install: $(tclmoduledir)/$(build_lib)
$(tclmoduledir)/$(build_lib): $(build_lib)
	$(INSTALL_DIR) $(tclmoduledir)
	$(INSTALL_LIB) $(build_lib) $(tclmoduledir)

################ Cleanup
clean: $(TCLMODULE)-lib-clean
$(TCLMODULE)-lib-clean:
	-$(RM) $(build_lib) $($(TCLMODULE)_OBJS)

################ Dependencies
DEP_SRC += $($(TCLMODULE)_SRC)
DEP_CPPFLAGS += $($(TCLMODULE)_CPPFLAGS) $($(TCLMODULE)_CFLAGS)

