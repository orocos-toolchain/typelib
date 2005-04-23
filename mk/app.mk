#
# $Id: app.mk,v 1.3 2003/10/07 15:30:45 fpy Exp $
#
# Copyright 2003 (c) CNRS/LAAS
#
# Author(s):
#  - Frederic Py <fpy@laas.fr>
#

APP_OBJS = $(APP_SRC:%.cc=%.o)

build: $(APP_NAME)
$(APP_NAME): $(APP_OBJS) $(APP_EXTRAS)
	$(LTLD) $(LDFLAGS) $(APP_LDFLAGS) -o $@ $(APP_OBJS) $(APP_EXTRAS) $(LIBS) $(APP_LIBS)

%.o: %.c
	$(CC)  $(CFLAGS) $(APP_CFLAGS) -c $(CPPFLAGS) $(APP_CPPFLAGS) $< -o $@ 

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) -c $(CPPFLAGS) $(APP_CPPFLAGS) $< -o $@ 

############### Clean
clean: app-clean
app-clean:
	$(LTRM) $(APP_NAME) $(APP_OBJS)

############### Install
install: app-install
app-install: $(APP_NAME)
	$(INSTALL_DIR) $(bindir)
	$(INSTALL_PROGRAM) $(APP_NAME) $(bindir)/$(APP_NAME)

############### Dependencies
DEP_SRC += $(APP_SRC)
DEP_CPPFLAGS += $(APP_CPPFLAGS)

