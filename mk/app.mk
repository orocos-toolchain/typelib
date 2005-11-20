# $Revision: 1056 $
# $Id: app.mk 1056 2005-10-13 08:40:19Z sjoyeux $

APP_OBJS = $(APP_SRC:%.cc=%.o)

$(APP_NAME)_LIB_DEPENDS=$(filter %.la,$($(MODULE)_LIBS))
ifneq ($($(APP_NAME)_LIB_DEPENDS),)
$($(APP_NAME)_LIB_DEPENDS): recurse-build
endif
recurse-build:

build: $(APP_NAME)
$(APP_NAME): DESCRIPTION='Linking application $(APP_NAME) (libtool)'
$(APP_NAME): $(APP_OBJS) $(APP_EXTRAS) $($(APP_NAME)_LIB_DEPENDS)
	$(COMMAND_PREFIX)$(LTLD) $(LDFLAGS) $(APP_LDFLAGS) -o $@ $(APP_OBJS) $(APP_EXTRAS) $(LIBS) $(APP_LIBS)

include $(top_srcdir)/mk/compile.mk

############### Clean
clean: app-clean
app-clean: DESCRIPTION='Cleaning $(PWD) (libtool)'
app-clean:
	$(COMMAND_PREFIX)$(LTRM) $(APP_NAME) $(APP_OBJS)

############### Install
install: app-install
app-install: DESCRIPTION='Installing $(APP_NAME) (libtool)'
app-install: $(APP_NAME)
	$(INSTALL_DIR) $(bindir)
	$(COMMAND_PREFIX)$(INSTALL_PROGRAM) $(APP_NAME) $(bindir)/$(APP_NAME)

############### Dependencies
DEP_SRC += $(APP_SRC)
DEP_CPPFLAGS += $(APP_CPPFLAGS)

