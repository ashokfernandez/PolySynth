# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/PolySynth/config
IPLUG2_ROOT = ../../../../external/iPlug2
include ../../../../external/iPlug2/common-web.mk

SRC += $(PROJECT_ROOT)/PolySynth.cpp

WAM_SRC += $(IPLUG_EXTRAS_PATH)/Synth/*.cpp
WAM_SRC += $(PROJECT_ROOT)/DemoSequencer.cpp

SEA_DSP_INCLUDE = $(PROJECT_ROOT)/../../../libs/SEA_DSP/include
UI_CONTROLS_INCLUDE = $(PROJECT_ROOT)/../../../UI/Controls

WAM_CFLAGS += -I$(IPLUG_SYNTH_PATH) -I$(SEA_DSP_INCLUDE)

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2 -I$(SEA_DSP_INCLUDE) -I$(UI_CONTROLS_INCLUDE)

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'ModuleFactory'" -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
