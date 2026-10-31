# ****************************************************************************
#    Badge Arcade: the games of BadgeOS, as a Ledger Nano S Plus app.
#    Built on the Ledger app-boilerplate Makefile (Apache 2.0).
# ****************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.target

########################################
#        Mandatory configuration       #
########################################
APPNAME = "Badge Arcade"

APPVERSION_M = 1
APPVERSION_N = 1
APPVERSION_P = 0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

APP_SOURCE_PATH += src

# The dashboard icon (14x14, 1 bit). The Nano home-screen icon is generated from it by the SDK.
ICON_NANOSP = icons/arcade_14px.gif
ICON_NANOX = icons/arcade_14px.gif
ICON_HOME_NANO = glyphs/home_arcade_14px.gif

# No keys, no coins: the app never derives a key, so it asks for no curve and no path.
CURVE_APP_LOAD_PARAMS =
PATH_APP_LOAD_PARAMS =

VARIANT_PARAM = APP
VARIANT_VALUES = arcade

#DEBUG = 1

########################################
# Application communication interfaces #
########################################
ENABLE_NBGL_FOR_NANO_DEVICES = 1

########################################
#              Storage                 #
########################################
# A few bytes of flash for the high scores: one uint32 per game (see src/storage.c).
ENABLE_APP_STORAGE = 1
APP_STORAGE_SIZE = 32
ENABLE_APP_STORAGE_PROP_SETTINGS = 0
ENABLE_APP_STORAGE_PROP_DATA = 1

ENABLE_STACK_PROTECTOR = 1

include $(BOLOS_SDK)/Makefile.standard_app
