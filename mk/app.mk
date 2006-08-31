# $Revision: 1381 $
# $Id: app.mk 1381 2006-02-07 13:46:29Z sjoyeux $

APP_OBJS = $(APP_SRC:%.cc=%.lo)

$(APP_NAME)_LIB_DEPENDS=$(filter %.la,$(APP_LIBS))
ifneq ($($(APP_NAME)_LIB_DEPENDS),)
$($(APP_NAME)_LIB_DEPENDS): | recurse-build
endif
recurse-build:

build: $(APP_NAME)
$(APP_NAME): DESCRIPTION='Linking application $(APP_NAME) (libtool)'
$(APP_NAME): PREFER_NON_PIC=-prefer-non-pic
$(APP_NAME): MODULE=APP
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
ifeq ($(APP_INSTALL),no)
app-install:
else
app-install: DESCRIPTION='Installing $(APP_NAME) (libtool)'
app-install: $(APP_NAME)
	$(INSTALL_DIR) $(DESTDIR)$(bindir)
	$(COMMAND_PREFIX)$(INSTALL_PROGRAM) $(APP_NAME) $(DESTDIR)$(bindir)/$(APP_NAME)
endif

############### Dependencies
DEP_SRC += $(APP_SRC)
DEP_CPPFLAGS += $(APP_CPPFLAGS)

