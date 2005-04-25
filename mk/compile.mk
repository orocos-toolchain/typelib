
%.lo: DESCRIPTION="Compiling $(notdir $@) (libtool)"

%.lo: %.c
	$(COMMAND_PREFIX)$(LTCC) $(CFLAGS) $($(MODULE)_CXXFLAGS) -c $(CPPFLAGS) $($(MODULE)_CPPFLAGS) -o $@ $<

%.lo: %.cc
	$(COMMAND_PREFIX)$(LTCXX) $(CXXFLAGS) $($(MODULE)_CXXFLAGS) -c $(CPPFLAGS) $($(MODULE)_CPPFLAGS) -o $@ $<

%.o: %.c
	$(COMMAND_PREFIX)$(CC)  $(CFLAGS) $(APP_CFLAGS) -c $(CPPFLAGS) $(APP_CPPFLAGS) $< -o $@ 

%.o: %.cc
	$(COMMAND_PREFIX)$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) -c $(CPPFLAGS) $(APP_CPPFLAGS) $< -o $@ 

