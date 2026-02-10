# IPLUG2_ROOT should point to the top level iPlug2 folder from the project folder.
IPLUG2_ROOT = ../../external/iPlug2

include ../../external/iPlug2/common-web.mk

SRC += $(PROJECT_ROOT)/ComponentGallery.cpp

# Add UI/Controls include path
WAM_CFLAGS += -I../../UI/Controls -DIPLUG_WEB=1
WEB_CFLAGS += -I../../UI/Controls -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2 -DIPLUG_WEB=1

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'ModuleFactory'" -s ASSERTIONS=0
WEB_LDFLAGS += -O3 -s ASSERTIONS=0 -s SINGLE_FILE=1
WEB_LDFLAGS += $(NANOVG_LDFLAGS)
