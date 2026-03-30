#include "ReaConvert_LoadAPI.h"

// Define the storage for the REAPER API function pointers.
#define REAPERAPI_IMPLEMENT

// We use the definitions from CMake for REAPERAPI_WANT_xxx
#include "reaper_plugin_functions.h"

int ReaConvert_LoadAPI(void *(*getFunc)(const char *)) {
    return REAPERAPI_LoadAPI(getFunc);
}
