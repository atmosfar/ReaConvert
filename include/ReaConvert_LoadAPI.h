#ifndef REACONVERT_API_H
#define REACONVERT_API_H

#ifdef __cplusplus
extern "C" {
#endif

int ReaConvert_LoadAPI(void *(*getFunc)(const char *));

#ifdef __cplusplus
}
#endif

#endif // REACONVERT_API_H
