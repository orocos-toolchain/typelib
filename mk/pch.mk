# $Revision: 1255 $
# $Id: pch.mk 1255 2006-01-18 10:58:25Z sjoyeux $

ifeq ($(USE_PCH),yes)
  PCH_DIR=     $(abs_top_builddir)/$(PCH_HEADER).gch
  PCH_CFLAGS=   -include $(top_builddir)/$(PCH_HEADER)
  
  CXXFLAGS_NOPCH := $(CXXFLAGS)
  CXXFLAGS += -Winvalid-pch $(PCH_CFLAGS) 

build: build-pch
recurse-build: build-pch

build-pch: build-pch-pic build-pch-nopic
build-pch-pic: DESCRIPTION="Building PCH from $(PCH_HEADER), with options -fPIC -DPIC $(CXXFLAGS_NOPCH)"
build-pch-pic: $(PCH_DIR)/stamp-pic
build-pch-nopic: DESCRIPTION="Building PCH from $(PCH_HEADER), with options $(CXXFLAGS_NOPCH)"
build-pch-nopic: $(PCH_DIR)/stamp-nopic
	
$(PCH_DIR)/stamp-pic: $(abs_top_srcdir)/$(PCH_HEADER) 
	-@mkdir -p $(PCH_DIR)
	$(COMMAND_PREFIX)$(CXX) $(CPPFLAGS) $(PCH_CPPFLAGS) $(CXXFLAGS_NOPCH) \
		-fPIC -DPIC -x c++ -c -o $(PCH_DIR)/cxx-pic $(abs_top_srcdir)/$(PCH_HEADER)
	@touch  $(PCH_DIR)/stamp-pic

$(PCH_DIR)/stamp-nopic: $(abs_top_srcdir)/$(PCH_HEADER) 
	-@mkdir -p $(PCH_DIR)
	$(COMMAND_PREFIX)$(CXX) $(CPPFLAGS) $(PCH_CPPFLAGS) $(CXXFLAGS_NOPCH) \
		-x c++ -c -o $(PCH_DIR)/cxx-nopic $(abs_top_srcdir)/$(PCH_HEADER)
	@touch  $(PCH_DIR)/stamp-nopic

else
build-pch:
endif

pch-clean:
	@rm -fR $(PCH_DIR)

