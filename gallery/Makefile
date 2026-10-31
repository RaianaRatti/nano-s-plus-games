# ****************************************************************************
#    Curio: a pocket gallery of lesser-known art, as a Ledger Nano S Plus app.
#    Built on the Ledger app-boilerplate Makefile (Apache 2.0).
# ****************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.target

########################################
#        Mandatory configuration       #
########################################
# The name on the dashboard. The scripts read it from here (the READMEs say it too).
APPNAME = "Curio"

APPVERSION_M = 1
APPVERSION_N = 0
APPVERSION_P = 0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# The gallery (gallery/src; works.c is made by tools/make_gallery.py), on the arcade's core.
APP_SOURCE_PATH += src
APP_SOURCE_FILES += ../src/gfx.c ../src/ui.c ../src/input.c ../src/random.c ../src/main.c
INCLUDES_PATH += ../src

# The dashboard icon (14x14, 1 bit). The Nano home-screen icon is generated from it by the SDK.
ICON_NANOSP = icons/gallery_14px.gif
ICON_NANOX = icons/gallery_14px.gif
ICON_HOME_NANO = glyphs/home_gallery_14px.gif

# No keys, no coins: the app never derives a key, so it asks for no curve and no path.
CURVE_APP_LOAD_PARAMS =
PATH_APP_LOAD_PARAMS =

VARIANT_PARAM = APP
VARIANT_VALUES = gallery

#DEBUG = 1

########################################
# Application communication interfaces #
########################################
ENABLE_NBGL_FOR_NANO_DEVICES = 1

########################################
#              Storage                 #
########################################
# The work last seen (see gallery/src/gallery.c): 2 bytes.
ENABLE_APP_STORAGE = 1
APP_STORAGE_SIZE = 8
ENABLE_APP_STORAGE_PROP_SETTINGS = 0
ENABLE_APP_STORAGE_PROP_DATA = 1

ENABLE_STACK_PROTECTOR = 1

include $(BOLOS_SDK)/Makefile.standard_app
