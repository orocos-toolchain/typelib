include $(abs_top_builddir)/mk/init.mk

ifneq ($(USE_PCH),)
  PCH_FILE=     $(abs_top_builddir)/$(PCH_HEADER).gch
  PCH_CFLAGS=   -include $(top_builddir)/$(PCH_HEADER)
  
  CXXFLAGS_NOPCH := $(CXXFLAGS)
  CXXFLAGS +=  $(PCH_CFLAGS) -Winvalid-pch
  PCH_DEP=$(PCH_FILE)
else
  PCH_DEP=
endif

@abs_builddir@/%.h.gch: @abs_srcdir@/%.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS_NOPCH) -fPIC -DPIC -x c++ -c -o $@ $<

build: $(PCH_DEP)

