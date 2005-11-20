# $Revision: 1022 $
# $Id: compile.mk 1022 2005-10-11 12:06:01Z sjoyeux $

%.lo: DESCRIPTION="Compiling $(notdir $@) (libtool)"
	
%.lo: %.c
	$(COMMAND_PREFIX)$(LTCC) $(CFLAGS) $($(MODULE)_CXXFLAGS) -c $(CPPFLAGS) $($(MODULE)_CPPFLAGS) -o $@ $<
	
%.lo: %.cc
	$(COMMAND_PREFIX)$(LTCXX) $(CXXFLAGS) $($(MODULE)_CXXFLAGS) -c $(CPPFLAGS) $($(MODULE)_CPPFLAGS) -o $@ $<

%.o: DESCRIPTION="Compiling $(notdir $@)"
	
%.o: %.c
	$(COMMAND_PREFIX)$(CC)  $(CFLAGS) $(APP_CFLAGS) -c $(CPPFLAGS) $(APP_CPPFLAGS) $< -o $@ 

%.o: %.cc
	$(COMMAND_PREFIX)$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) -c $(CPPFLAGS) $(APP_CPPFLAGS) $< -o $@ 

