# IPLUG2_ROOT should point to the top level iPlug2 folder from the project folder.
IPLUG2_ROOT = ../../external/iPlug2

include ../../external/iPlug2/common-web.mk

SRC += $(PROJECT_ROOT)/ComponentGallery.cpp

WAM_CFLAGS += -DIPLUG_WEB=1
WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2 -DIPLUG_WEB=1

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'ModuleFactory'" -s ASSERTIONS=0
WEB_LDFLAGS += -O3 -s ASSERTIONS=0
WEB_LDFLAGS += $(NANOVG_LDFLAGS)
