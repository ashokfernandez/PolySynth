#define PLUG_NAME "ComponentGallery"
#define PLUG_MFR "MyCompany"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'CGal'
#define PLUG_MFR_ID 'MyCo'
#define PLUG_URL_STR "https://example.com"
#define PLUG_EMAIL_STR "dev@example.com"
#define PLUG_COPYRIGHT_STR "Copyright 2026 MyCompany"
#define PLUG_CLASS_NAME ComponentGallery

#define BUNDLE_NAME "gallery"
#define BUNDLE_MFR "mycompany"
#define BUNDLE_DOMAIN "com"

#define PLUG_CHANNEL_IO "0-2"
#define SHARED_RESOURCES_SUBPATH "ComponentGallery"

#define PLUG_LATENCY 0
#define PLUG_TYPE 1
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 1024
#define PLUG_HEIGHT 768
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY ComponentGallery_Entry
#define AUV2_ENTRY_STR "ComponentGallery_Entry"
#define AUV2_FACTORY ComponentGallery_Factory
#define AUV2_VIEW_CLASS ComponentGallery_View
#define AUV2_VIEW_CLASS_STR "ComponentGallery_View"

#define AAX_TYPE_IDS 'CGA1', 'CGA2'
#define AAX_PLUG_MFR_STR "MyCompany"
#define AAX_PLUG_NAME_STR "ComponentGallery\nCGAL"
#define AAX_DOES_AUDIOSUITE 0
#define AAX_PLUG_CATEGORY_STR "Effect"

#define VST3_SUBCATEGORY "Fx"
#define CLAP_MANUAL_URL "https://example.com/manual"
#define CLAP_SUPPORT_URL "https://example.com/support"
#define CLAP_DESCRIPTION "Component gallery visual test plugin"
#define CLAP_FEATURES "audio-effect"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"
#define ROBOTO_BOLD_FN "Roboto-Bold.ttf"

#define kCtrlTag_TestKnob 101
